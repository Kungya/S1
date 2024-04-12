// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Spearman/SpearComponents/CombatComponent.h"
#include "Spearman/Weapon/Weapon.h"
#include "Net/UnrealNetwork.h"
#include "SpearmanCharacterAnimInstance.h"
#include "Kismet/KIsmetMathLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Spearman/Spearman.h"
#include "Camera/PlayerCameraManager.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Spearman/GameMode/SpearmanGameMode.h"
#include "Components/WidgetComponent.h"
#include "Spearman/HUD/HitDamageWidget.h"
#include "Kismet/GamePlayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Spearman/HUD/HpBarWidget.h"


ASpearmanCharacter::ASpearmanCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 250.f;
	CameraBoom->SocketOffset = FVector(0.f, 75.f, 75.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 마우스 회전
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	HitDamage = CreateDefaultSubobject<UWidgetComponent>(TEXT("HitDamageWidget"));
	HitDamage->SetupAttachment(GetMesh());
	HitDamage->SetWidgetSpace(EWidgetSpace::Screen);

	HpBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBarWidget"));
	HpBar->SetupAttachment(GetMesh());
	HpBar->SetWidgetSpace(EWidgetSpace::Screen);
	HpBar->SetDrawSize(FVector2D(125.f, 20.f));

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);

	TIPState = ETurnInPlace::ETIP_NotTurn;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	GetCharacterMovement()->JumpZVelocity = 480.f;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
}

void ASpearmanCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASpearmanCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ASpearmanCharacter, Hp);
}

void ASpearmanCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}

	GetMesh()->HideBoneByName(TEXT("weapon"), EPhysBodyOp::PBO_None);

	// TODO : if statement IsLocallyControleld
	HitDamage->InitWidget();
	HitDamageWidget = Cast<UHitDamageWidget>(HitDamage->GetUserWidgetObject());
	
	HpBar->InitWidget();
	HpBarWidget = Cast<UHpBarWidget>(HpBar->GetUserWidgetObject());
}

void ASpearmanCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetMesh()->HideBoneByName(TEXT("sword_bottom"), EPhysBodyOp::PBO_None);

	SpearmanPlayerController = SpearmanPlayerController == nullptr ? Cast<ASpearmanPlayerController>(Controller) : SpearmanPlayerController;
	if (SpearmanPlayerController)
	{
		if (SpearmanPlayerController->PlayerCameraManager)
		{
			SpearmanPlayerController->PlayerCameraManager->ViewPitchMin = -45.f;
			SpearmanPlayerController->PlayerCameraManager->ViewPitchMax = 45.f;
		}
	}

	ShowHitDamage(false);

	UpdateHUDHp();

	HpBarWidget->SetHpBar(GetHpRatio());
	HpBar->SetVisibility(false);

	if (IsLocallyControlled())
	{
		HpBar->SetVisibility(false);
	}

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ASpearmanCharacter::OnAttacked);
	}
}

void ASpearmanCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastMovementReplication += DeltaTime;
	if (TimeSinceLastMovementReplication > 0.25f)
	{
		OnRep_ReplicatedMovement();
	}

	CalculateAO_Pitch();
	HideCameraIfCharacterTooClose();
}

void ASpearmanCharacter::PlaySpearAttackMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SpearAttackMontage)
	{
		AnimInstance->Montage_Play(SpearAttackMontage);
	}
}

void ASpearmanCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	Combat->CombatState = ECombatState::ECS_Stunned;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ASpearmanCharacter::PlayDeathMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);
		// TODO : 기능을 추가한다면 피격 방향에 따른 JumpToSection
	}
}

void ASpearmanCharacter::OnAttacked(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{ // in server
	Hp = FMath::Clamp(Hp - Damage, 0.f, MaxHp);
	UpdateHUDHp();

	if (FMath::IsNearlyZero(Hp))
	{
		ASpearmanGameMode* SpearmanGameMode = GetWorld()->GetAuthGameMode<ASpearmanGameMode>();
		if (SpearmanGameMode)
		{
			SpearmanPlayerController = SpearmanPlayerController == nullptr ? Cast<ASpearmanPlayerController>(Controller) : SpearmanPlayerController;
			ASpearmanPlayerController* AttackerController = Cast<ASpearmanPlayerController>(InstigatorController);
			SpearmanGameMode->PlayerDeath(this, SpearmanPlayerController, AttackerController);
		}
	}
	else
	{
		PlayHitReactMontage();
	}
}

void ASpearmanCharacter::OnRep_Hp(float LastHp)
{
	if (!IsLocallyControlled())
	{ // TODO : 피격자가 입은 데미지만 공격자 입장에서 표시 -> 우선은 피격자, 서버를 제외하고 전부 표시
		const float Damage = LastHp - Hp;
		HitDamageWidget->SetHitDamageText(Damage);
		ShowHitDamage(true);
		HpBarWidget->SetHpBar(GetHpRatio());

		GetWorldTimerManager().SetTimer(HitDamageTimerHandle, this, &ASpearmanCharacter::HideHitDamage, 2.f, false);
	}
	UpdateHUDHp();
	if (FMath::IsNearlyZero(Hp))
	{ // TODO : Die client widget
		HideHpBar();
	}
	else
	{
		PlayHitReactMontage();
	}
}

void ASpearmanCharacter::WeaponHit_Implementation(FHitResult HitResult)
{
	if (HitParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticles, HitResult.Location, FRotator(0.f), true);
	}

	if (!IsLocallyControlled())
	{
		ShowHpBar();
	}
}

void ASpearmanCharacter::UpdateHUDHp()
{
	SpearmanPlayerController = SpearmanPlayerController == nullptr ? Cast<ASpearmanPlayerController>(Controller) : SpearmanPlayerController;
	if (SpearmanPlayerController)
	{
		SpearmanPlayerController->SetHUDHp(Hp, MaxHp);
	}
}
void ASpearmanCharacter::ShowHitDamage(bool bShowHitDamage)
{
	if (HitDamage)
	{
		HitDamage->SetVisibility(bShowHitDamage);
		// TODO : Animation
	}
}

void ASpearmanCharacter::HideHitDamage()
{
	if (HitDamage)
	{
		HitDamage->SetVisibility(false);
	}
}

void ASpearmanCharacter::ShowHpBar()
{
	HpBar->SetVisibility(true);
	GetWorldTimerManager().ClearTimer(HpBarTimer);
	GetWorldTimerManager().SetTimer(HpBarTimer, this, &ASpearmanCharacter::HideHpBar, HpBarDisplayTime);
}

void ASpearmanCharacter::HideHpBar()
{
	HpBar->SetVisibility(false);
}

void ASpearmanCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	TurnInPlace();
	TimeSinceLastMovementReplication = 0.f;
}

void ASpearmanCharacter::Death()
{ // server only
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	MulticastDeath();
	GetWorldTimerManager().SetTimer(DeathTimer, this, &ASpearmanCharacter::DeathTimerFinished, DeathDelay);
}

void ASpearmanCharacter::MulticastDeath_Implementation()
{
	bDeath = true;
	PlayDeathMontage();

	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (SpearmanPlayerController)
	{
		DisableInput(SpearmanPlayerController);
	}
	// Disable Collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASpearmanCharacter::DeathTimerFinished()
{
	ASpearmanGameMode* SpearmanGameMode = GetWorld()->GetAuthGameMode<ASpearmanGameMode>();
	if (SpearmanGameMode)
	{
		SpearmanGameMode->RequestRespawn(this, Controller);
	}
}

void ASpearmanCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// [270, 360) -> [-90, 0)
		AO_Pitch -= 360.f;
	}
	// 상하 Weapon각 제한
	AO_Pitch = FMath::ClampAngle(AO_Pitch, -45.f, 45.f);
}

void ASpearmanCharacter::TurnInPlace()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();

	if (Speed > 0.f)
	{
		TIPState = ETurnInPlace::ETIP_NotTurn;
		return;
	}

	RotationPrevFrame = RotationNowFrame;
	RotationNowFrame = GetBaseAimRotation();
	YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(RotationNowFrame, RotationPrevFrame).Yaw;

	if (FMath::Abs(YawOffset) > TurnThreshold)
	{
		if (YawOffset > TurnThreshold)
			TIPState = ETurnInPlace::ETIP_Right;
		else if (YawOffset < -TurnThreshold)
			TIPState = ETurnInPlace::ETIP_Left;
		else
			TIPState = ETurnInPlace::ETIP_NotTurn;

		return;
	}
	else
		TIPState = ETurnInPlace::ETIP_NotTurn;
}

void ASpearmanCharacter::ServerEquipButtonPressed_Implementation()
{ // server-side
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ASpearmanCharacter::AttackButtonPressed()
{
	if (Combat && Combat->CombatState == ECombatState::ECS_Idle)
	{ // Client측 bCanAttack이 변조되더라도 Server내 에서 bCanAttack 재검사 후 실행 결정
		Combat->ServerSpearAttack();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't Attack"));
	}
}

void ASpearmanCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{ // Server에서 직접 조종하는 케이스 처리
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ASpearmanCharacter::IsWeaponEquipped()
{ // bool로 AnimInstance에서 사용, AnimBP를 위함임
	return (Combat && Combat->EquippedWeapon);
}

void ASpearmanCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{  // OverlappingWeapon이 다시 nullptr로 replicated 되기 직전 마지막 값
		LastWeapon->ShowPickupWidget(false);
	}
}

void ASpearmanCharacter::HideCameraIfCharacterTooClose()
{ // client-side
	if (!IsLocallyControlled()) return;
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < 100.f)
	{
		GetMesh()->SetVisibility(false);
		if (GetCombat() && GetCombat()->EquippedWeapon)
			GetCombat()->EquippedWeapon->SetWeaponVisibility(false);
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (GetCombat() && GetCombat()->EquippedWeapon)
			GetCombat()->EquippedWeapon->SetWeaponVisibility(true);
	}
}

void ASpearmanCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASpearmanCharacter::Jump);
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &ASpearmanCharacter::DashButtonPressed);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ASpearmanCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ASpearmanCharacter::AttackButtonPressed);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASpearmanCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASpearmanCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ASpearmanCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ASpearmanCharacter::LookUp);
}

void ASpearmanCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ASpearmanCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ASpearmanCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ASpearmanCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ASpearmanCharacter::Jump()
{
	Super::Jump();

}

void ASpearmanCharacter::DashButtonPressed()
{
	if (Combat == nullptr) return;


	if (Combat->bCanDash == true && Combat->CombatState == ECombatState::ECS_Idle)
	{
		FVector InputVector = GetLastMovementInputVector().GetSafeNormal();
		Combat->DashButtonPressed(InputVector);
	}
}

void ASpearmanCharacter::EquipButtonPressed()
{
	if (Combat)
	{
		if (HasAuthority())
		{ // 서버에서 조종할 때 처리
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{ // RPC,
			ServerEquipButtonPressed();
		}
	}
}