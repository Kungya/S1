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

	// ���콺 ȸ��
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	HitDamage = CreateDefaultSubobject<UWidgetComponent>(TEXT("HitDamageWidget"));
	HitDamage->SetupAttachment(GetMesh());
	HitDamage->SetWidgetSpace(EWidgetSpace::Screen);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

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

	HitDamage->InitWidget();

	HitDamageWidget = Cast<UHitDamageWidget>(HitDamage->GetUserWidgetObject());
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
		// TODO : �ǰ� ���⿡ ���� JumpToSection
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
	{ // TODO : �ǰ��ڰ� ���� �������� ������ ���忡�� ǥ�� -> �켱�� �ǰ���, ������ �����ϰ� ���� ǥ��
		const float Damage = LastHp - Hp;
		HitDamageWidget->SetHitDamageText(Damage);
		ShowHitDamage(true);

		GetWorldTimerManager().SetTimer(HitDamageTimerHandle, this, &ASpearmanCharacter::HideHitDamage, 2.f, false);
	}
	UpdateHUDHp();
	if (!FMath::IsNearlyZero(Hp))
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
}

void ASpearmanCharacter::UpdateHUDHp()
{
	SpearmanPlayerController = SpearmanPlayerController == nullptr ? Cast<ASpearmanPlayerController>(Controller) : SpearmanPlayerController;
	if (SpearmanPlayerController)
	{
		SpearmanPlayerController->SetHUDHp(Hp, MaxHp);
	}
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
	GetWorldTimerManager().SetTimer(
		DeathTimer,
		this,
		&ASpearmanCharacter::DeathTimerFinished,
		DeathDelay
	);
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

void ASpearmanCharacter::ShowHitDamage(bool bShowHitDamage)
{
	if (HitDamage)
	{
		HitDamage->SetVisibility(bShowHitDamage);
		// TODO : Add Location per tick
	}
}

void ASpearmanCharacter::HideHitDamage()
{
	HitDamage->SetVisibility(false);
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

void ASpearmanCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// [270, 360) -> [-90, 0)
		AO_Pitch -= 360.f;
	}
	// ���� Weapon�� ����
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
	if (Combat && Combat->bCanAttack)
	{ // Client�� bCanAttack�� �����Ǵ��� Server�� ���� bCanAttack ��˻� �� ���� ����
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
	{ // Server���� ���� �����ϴ� ���̽� ó��
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ASpearmanCharacter::IsWeaponEquipped()
{ // bool�� AnimInstance���� ���, AnimBP�� ������
	return (Combat && Combat->EquippedWeapon);
}

void ASpearmanCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{ // OverlappingWeapon�� �ٽ� nullptr�� replicated �Ǳ� ���� ������ ��
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
	if (Combat && Combat->bCanDash == true)
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
		{ // �������� ������ �� ó��
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{ // RPC,
			ServerEquipButtonPressed();
		}
	}
}