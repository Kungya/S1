// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "PaperSpriteComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Spearman/SpearComponents/CombatComponent.h"
#include "Spearman/SpearComponents/BuffComponent.h"
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
#include "Spearman/AI/BasicMonsterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Spearman/Items/Item.h"
#include "Spearman/SpearComponents/InventoryComponent.h"
#include "Spearman/HUD/CharacterOverlay.h"
#include "Spearman/HUD/InventoryHUD/S1InventoryWidget.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Spearman/SpearComponents/LagCompensationComponent.h"
#include "Spearman/HUD/SpearmanHUD.h"
#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Spearman/DamageType/BlueZoneDamageType.h"

FOnSpearmanCharacterSwapWeapon ASpearmanCharacter::NotifySwapWeapon;

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

	MinimapSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("MinimapSpringArm"));
	MinimapSpringArm->SetupAttachment(GetMesh());
	MinimapSpringArm->TargetArmLength = 300.f;
	MinimapSpringArm->SetRelativeRotation(FQuat(FRotator(270.f, 0.f, 0.f)));
	MinimapSpringArm->bUsePawnControlRotation = false;

	MinimapSceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapSceneCapture"));
	MinimapSceneCapture->SetupAttachment(MinimapSpringArm, USpringArmComponent::SocketName);
	MinimapSceneCapture->SetIsReplicated(false);

	MinimapCursorSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("MinimapCursor"));
	MinimapCursorSprite->SetupAttachment(GetMesh());
	MinimapCursorSprite->bVisibleInSceneCaptureOnly = true;
	MinimapCursorSprite->bOwnerNoSee = true;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensationComponent"));

	HitDamage = CreateDefaultSubobject<UWidgetComponent>(TEXT("HitDamageWidget"));
	HitDamage->SetupAttachment(GetMesh());
	HitDamage->SetWidgetSpace(EWidgetSpace::Screen);

	HpBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBarWidget"));
	HpBar->SetupAttachment(GetMesh());
	HpBar->SetWidgetSpace(EWidgetSpace::Screen);
	HpBar->SetDrawSize(FVector2D(125.f, 20.f));

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	TIPState = ETurnInPlace::ETIP_NotTurn;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	GetCharacterMovement()->JumpZVelocity = 480.f;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	NetCullDistanceSquared = 4'000'000.f;
	bNetLoadOnClient = false;

	/*
	* Hit Box, WARNING : Never RELOCATE HitBox's Order, "HitBoxArray[0] => head" (0 index is head)
	*/

	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	head->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	HitBoxArray.Add(head);

	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	pelvis->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(pelvis);

	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	spine_02->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(spine_02);

	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	spine_03->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(spine_03);

	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	upperarm_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(upperarm_l);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	upperarm_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(upperarm_r);

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	lowerarm_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(lowerarm_l);

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	lowerarm_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(lowerarm_r);

	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	hand_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(hand_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	hand_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(hand_r);

	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	thigh_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(thigh_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	thigh_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(thigh_r);

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	calf_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(calf_l);

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	calf_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(calf_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	foot_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(foot_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	foot_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(foot_r);
}

void ASpearmanCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASpearmanCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ASpearmanCharacter, Hp);
	DOREPLIFETIME(ASpearmanCharacter, bDisableKeyInput);
	DOREPLIFETIME_CONDITION(ASpearmanCharacter, bIsInBlueZone, COND_OwnerOnly);
}

void ASpearmanCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}

	if (Buff)
	{
		Buff->Character = this;
	}

	if (LagCompensation)
	{
		LagCompensation->AttackerSpearmanCharacter = this;
	}

	GetMesh()->HideBoneByName(TEXT("weapon"), EPhysBodyOp::PBO_None);

	HitDamage->InitWidget();
	HitDamageWidget = Cast<UHitDamageWidget>(HitDamage->GetUserWidgetObject());
	
	HpBar->InitWidget();
	HpBarWidget = Cast<UHpBarWidget>(HpBar->GetUserWidgetObject());
}

void ASpearmanCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpearmanPlayerController = (SpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(Controller) : SpearmanPlayerController;
	if (SpearmanPlayerController)
	{
		if (SpearmanPlayerController->PlayerCameraManager)
		{
			SpearmanPlayerController->PlayerCameraManager->ViewPitchMin = -45.f;
			SpearmanPlayerController->PlayerCameraManager->ViewPitchMax = 45.f;
		}
	}

	UpdateHUDHp();

	HitDamageWidget->SetVisibility(ESlateVisibility::Collapsed);

	HpBarWidget->SetHpBar(GetHpRatio());
	HpBarWidget->SetVisibility(ESlateVisibility::Collapsed);

	InitRenderTargetIfOwningClient();

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ASpearmanCharacter::OnAttacked);

		GetWorld()->GetTimerManager().SetTimer(BlueZoneTimerHandle, this, &ASpearmanCharacter::TakeDamageIfNotInBlueZone, 1.f, true);
	}

	/* Temporary, test for rewind */
	GetWorldTimerManager().SetTimer(TestTimer, this, &ASpearmanCharacter::TestToggleVector, 2.f, true);

	UE_LOG(LogTemp, Warning, TEXT("NetCullDist : %f"), NetCullDistanceSquared);
}

void ASpearmanCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bMove)
	{ /* Temporary, Test for Rewind */
		FVector MoveVector;
		MoveVector = GetActorForwardVector() * TimerVector;
		AddMovementInput(MoveVector);
	}

	if (bTestAttack)
	{
		bTestAttack = false;
		AttackButtonPressed();
	}

	if (HasAuthority())
	{
		FVector Velocity = GetVelocity();
		if (Velocity.Size() > 10.f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Velocity : %f : X:%f, Y:%f, Z:%f"), Velocity.Size(), Velocity.X, Velocity.Y, Velocity.Z);
		}
	}

	TimeSinceLastMovementReplication += DeltaTime;
	if (TimeSinceLastMovementReplication > 0.05f)
	{
		TimeSinceLastMovementReplication = 0.f;
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

void ASpearmanCharacter::PlayThrustMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrustMontage)
	{
		AnimInstance->Montage_Play(ThrustMontage);
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
	}
}

void ASpearmanCharacter::PlayDashMontage(const bool bLeft)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DashMontage)
	{
		AnimInstance->Montage_Play(DashMontage);
		FName SectionName = (bLeft == true) ? "DashLeft" : "DashRight";
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ASpearmanCharacter::PlayParriedMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DashMontage)
	{
		AnimInstance->Montage_Play(ParriedMontage);
	}
}

void ASpearmanCharacter::PlayStartDefenseMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DefenseMontage)
	{
		AnimInstance->Montage_Play(DefenseMontage);
	}
}

void ASpearmanCharacter::PlayEndDefenseMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DefenseMontage && AnimInstance->Montage_IsPlaying(DefenseMontage))
	{
		FName SectionName = "EndDefense";
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ASpearmanCharacter::OnAttacked(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{ /* Server Only */
	if (bDeath) return;

	Hp = FMath::Clamp(Hp - Damage, 0.f, MaxHp);
	UpdateHUDHp();

	if (FMath::IsNearlyZero(Hp))
	{
		HideHpBar();
		Death();

		ABasicMonsterAIController*BasicMonsterAIController = Cast<ABasicMonsterAIController>(InstigatorController);
		if (BasicMonsterAIController)
		{
			BasicMonsterAIController->GetBlackboardComponent()->SetValueAsBool(FName(TEXT("CharacterDead")), true);
		}
	}
	else
	{
		if (!DamageType->IsA<UBlueZoneDamageType>())
		{
			if (GetCombat() && GetCombat()->CombatState != ECombatState::ECS_SuperArmor)
			{
				GetCombat()->CombatState = ECombatState::ECS_Stunned;
				MulticastHitReact();
			}
		}
	}
}

void ASpearmanCharacter::OnRep_Hp(float LastHp)
{
	UpdateHUDHp();

	if (FMath::IsNearlyZero(Hp))
	{
		HideHpBar();
	}
}

void ASpearmanCharacter::MulticastHitReact_Implementation()
{
	PlayHitReactMontage();
}

void ASpearmanCharacter::WeaponHitEffect(int32 Damage, FVector_NetQuantize HitPoint, bool bHeadShot)
{ /* Interface */
	if (HitParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticles, HitPoint, FRotator(0.f), true);
	}

	if (!IsLocallyControlled())
	{
		HitDamageWidget->SetHitDamageText(Damage);
		ShowHitDamage();
		HpBarWidget->SetHpBar(GetHpRatio());
		ShowHpBar();
	}
}

void ASpearmanCharacter::UpdateHUDHp()
{
	SpearmanPlayerController = (SpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(Controller) : SpearmanPlayerController;
	if (SpearmanPlayerController)
	{
		SpearmanPlayerController->SetHUDHp(Hp, MaxHp);
	}
}

void ASpearmanCharacter::ShowHitDamage()
{
	if (HitDamageWidget)
	{
		HitDamageWidget->SetVisibility(ESlateVisibility::Visible);
	}

	GetWorldTimerManager().ClearTimer(HitDamageTimer);
	GetWorldTimerManager().SetTimer(HitDamageTimer, this, &ASpearmanCharacter::HideHitDamage, HitDamageDisplayTime);
}

void ASpearmanCharacter::HideHitDamage()
{
	if (HitDamageWidget && HpBarWidget)
	{
		HitDamageWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ASpearmanCharacter::ShowHpBar()
{
	if (HpBarWidget)
	{
		HpBarWidget->SetVisibility(ESlateVisibility::Visible);
	}

	GetWorldTimerManager().ClearTimer(HpBarTimer);
	GetWorldTimerManager().SetTimer(HpBarTimer, this, &ASpearmanCharacter::HideHpBar, HpBarDisplayTime);
}

void ASpearmanCharacter::HideHpBar()
{
	if (HpBarWidget)
	{
		HpBarWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ASpearmanCharacter::InitRenderTargetIfOwningClient()
{
	SpearmanPlayerController = (SpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(Controller) : SpearmanPlayerController;
	if (SpearmanPlayerController && SpearmanPlayerController->IsLocalController())
	{
		if (!HasAuthority())
		{
			RenderTargetMinimap = NewObject<UTextureRenderTarget2D>(this);
			RenderTargetMinimap->InitAutoFormat(1024, 1024);
			RenderTargetMinimap->UpdateResource();

			if (RenderTargetMinimap)
			{
				MinimapSceneCapture->TextureTarget = RenderTargetMinimap;
			}
		}
	}
}

void ASpearmanCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	TurnInPlace();
}

void ASpearmanCharacter::Extract()
{ /* Server Only */
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
		Combat->EquippedWeapon->SetbAttackCollisionTrace();
	}

	if (SpearmanPlayerController)
	{
		SpearmanPlayerController->HandleExtraction();
	}

	bDisableKeyInput = false;

	FTimerHandle ExtractTimer;
	GetWorldTimerManager().SetTimer(ExtractTimer, this, &ASpearmanCharacter::DestroyCallback, 1.f);
}

void ASpearmanCharacter::Death()
{ /* Server Only */
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
		Combat->EquippedWeapon->SetbAttackCollisionTrace();
	}

	if (SpearmanPlayerController)
	{ /* Spectate */
		SpearmanPlayerController->SetPlayerSpectate();
	}

	MulticastDeath();

	FTimerHandle DeathTimer;
	GetWorldTimerManager().SetTimer(DeathTimer, this, &ASpearmanCharacter::DestroyCallback, DeathDelay);
}

void ASpearmanCharacter::MulticastDeath_Implementation()
{
	bDeath = true;
	PlayDeathMontage();

	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	bDisableKeyInput = true;
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASpearmanCharacter::DestroyCallback()
{ /* Server Only */
	Destroy();
}

void ASpearmanCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{ // [270, 360) ->[-90, 0)
		AO_Pitch -= 360.f;
	}
	
	AO_Pitch = FMath::ClampAngle(AO_Pitch, -45.f, 45.f);
}

void ASpearmanCharacter::TurnInPlace()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	const float Speed = Velocity.Size();

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
	}
	else
		TIPState = ETurnInPlace::ETIP_NotTurn;
}

void ASpearmanCharacter::ServerEquipButtonPressed_Implementation()
{ /* Server Only */
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ASpearmanCharacter::AttackButtonPressed()
{
	if (bDisableKeyInput) return;
	if (Combat)
	{
		Combat->ServerSpearAttack();
	}
}

void ASpearmanCharacter::ThrustButtonPressed()
{
	if (bDisableKeyInput) return;
	if (Combat)
	{
		Combat->ServerThrust();
	}
}

void ASpearmanCharacter::DefenseButtonPressed()
{
	if (bDisableKeyInput) return;
	if (Combat)
	{
		Combat->ServerStartDefense();
	}
}

void ASpearmanCharacter::DefenseButtonReleased()
{
	if (bDisableKeyInput) return;
	if (Combat)
	{
		Combat->ServerEndDefense();
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
{
	return (Combat && Combat->EquippedWeapon);
}

void ASpearmanCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{  // Last Value before nullptr (OverlappingWeapon)
		LastWeapon->ShowPickupWidget(false);
	}
}

void ASpearmanCharacter::HideCameraIfCharacterTooClose()
{ /* Client Only */
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
	PlayerInputComponent->BindAction("Thrust", IE_Pressed, this, &ASpearmanCharacter::ThrustButtonPressed);
	PlayerInputComponent->BindAction("Defense", IE_Pressed, this, &ASpearmanCharacter::DefenseButtonPressed);
	PlayerInputComponent->BindAction("Defense", IE_Released, this, &ASpearmanCharacter::DefenseButtonReleased);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &ASpearmanCharacter::InteractButtonPressed);
	PlayerInputComponent->BindAction("Inventory", IE_Pressed, this, &ASpearmanCharacter::InventoryButtonPressed);
	PlayerInputComponent->BindAction("TriggerMove", IE_Pressed, this, &ASpearmanCharacter::TriggerMove);
	PlayerInputComponent->BindAction("TriggerAttack", IE_Pressed, this, &ASpearmanCharacter::StartAttackTest);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASpearmanCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASpearmanCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ASpearmanCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ASpearmanCharacter::LookUp);
}

void ASpearmanCharacter::MoveForward(float Value)
{
	if (bDisableKeyInput) return;
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ASpearmanCharacter::MoveRight(float Value)
{
	if (bDisableKeyInput) return;
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ASpearmanCharacter::Turn(float Value)
{
	if (bDisableKeyInput) return;
	AddControllerYawInput(Value);
}

void ASpearmanCharacter::LookUp(float Value)
{
	if (bDisableKeyInput) return;
	AddControllerPitchInput(Value);
}

void ASpearmanCharacter::Jump()
{
	if (bDisableKeyInput) return;
	Super::Jump();
}

void ASpearmanCharacter::DashButtonPressed()
{
	if (bDisableKeyInput) return;
	if (Combat == nullptr || !IsWeaponEquipped()) return;

	const FVector_NetQuantize InputVector = GetLastMovementInputVector().GetSafeNormal();
	Combat->ServerDash(InputVector);
}

void ASpearmanCharacter::EquipButtonPressed()
{
	if (bDisableKeyInput) return;
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

void ASpearmanCharacter::InteractButtonPressed()
{
	if (bDisableKeyInput) return;

	Interact();
}

void ASpearmanCharacter::Interact()
{
	ServerInteract();
}

void ASpearmanCharacter::ServerInteract_Implementation()
{ /* Server Only */
	FVector Start = FollowCamera->GetComponentLocation();
	FVector End = Start + FollowCamera->GetForwardVector() * 1'000.f;
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (IsWeaponEquipped())
	{
		Params.AddIgnoredActor(Combat->EquippedWeapon);
	}

	SpearmanPlayerController = (SpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(Controller) : SpearmanPlayerController;

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Interact, Params))
	{
		AItem* Item = Cast<AItem>(HitResult.GetActor());
		if (Item)
		{
			SpearmanPlayerController->GetInventory()->AddItem(Item->GetItemInstance());
		}

		IInteractableInterface* InteractableInterface = Cast<IInteractableInterface>(HitResult.GetActor());
		if (InteractableInterface)
		{
			InteractableInterface->Interact();
		}
	}
}

void ASpearmanCharacter::InventoryButtonPressed()
{
	SpearmanPlayerController = (SpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(Controller) : SpearmanPlayerController;
	if (SpearmanPlayerController)
	{
		SpearmanPlayerController->ShowInventoryWidget();
	}
}

void ASpearmanCharacter::TakeDamageIfNotInBlueZone()
{ /* Server Only */
	if (!bIsInBlueZone)
	{
		UGameplayStatics::ApplyDamage(this, 2.f, Controller, this, UBlueZoneDamageType::StaticClass());
	}
}

void ASpearmanCharacter::OnRep_bIsInBlueZone()
{
	ShowBlueZoneImage();
}

void ASpearmanCharacter::ShowBlueZoneImage()
{
	SpearmanPlayerController = (SpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(Controller) : SpearmanPlayerController;
	if (SpearmanPlayerController && SpearmanPlayerController->IsLocalController())
	{
		if (bIsInBlueZone)
		{
			if (SpearmanPlayerController->GetSpearmanHUD()->CharacterOverlay)
			{
				SpearmanPlayerController->GetSpearmanHUD()->CharacterOverlay->BlueZoneImage->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		else
		{
			SpearmanPlayerController->GetSpearmanHUD()->CharacterOverlay->BlueZoneImage->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

bool ASpearmanCharacter::GetIsPlayingDefenseMontage() const
{
	return GetMesh()->GetAnimInstance()->Montage_IsPlaying(DefenseMontage);
}
