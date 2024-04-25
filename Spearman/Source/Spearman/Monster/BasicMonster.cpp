// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicMonster.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/WidgetComponent.h"
#include "Spearman/HUD/HitDamageWidget.h"
#include "Spearman/HUD/HpBarWidget.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Spearman/AI/BasicMonsterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SphereComponent.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Components/BoxComponent.h"
#include "Spearman/Spearman.h"
#include "Kismet/GameplayStatics.h"
#include "Spearman/Weapon/Weapon.h"

ABasicMonster::ABasicMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	HitDamage = CreateDefaultSubobject<UWidgetComponent>(TEXT("HitDamageWidget"));
	HitDamage->SetupAttachment(GetMesh());

	HpBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBarWidget"));
	HpBar->SetupAttachment(GetMesh());
	HpBar->SetWidgetSpace(EWidgetSpace::Screen);
	HpBar->SetDrawSize(FVector2D(125.f, 20.f));

	AggroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AggroSphere"));
	AggroSphere->SetupAttachment(GetRootComponent());

	CombatRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackRangeSphere"));
	CombatRangeSphere->SetupAttachment(GetRootComponent());

	AttackCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("AttackCollisionBox"));
	AttackCollisionBox->SetupAttachment(GetMesh(), FName("Pig_Nose"));
	AttackCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttackCollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	AttackCollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AttackCollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Overlap);
}

void ABasicMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABasicMonster, Hp);
}

void ABasicMonster::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	HitDamage->InitWidget();
	HitDamageWidget = Cast<UHitDamageWidget>(HitDamage->GetUserWidgetObject());

	HpBar->InitWidget();
	HpBarWidget = Cast<UHpBarWidget>(HpBar->GetUserWidgetObject());
}

void ABasicMonster::BeginPlay()
{
	Super::BeginPlay();

	HitDamage->SetVisibility(false);

	HpBarWidget->SetHpBar(GetHpRatio());
	HpBar->SetVisibility(false);

	if (HasAuthority())
	{ // AI

		AggroSphere->OnComponentBeginOverlap.AddDynamic(this, &ABasicMonster::AggroSphereBeginOverlap);
		CombatRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &ABasicMonster::CombatRangeBeginOverlap);
		CombatRangeSphere->OnComponentEndOverlap.AddDynamic(this, &ABasicMonster::CombatRangeEndOverlap);
		AttackCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ABasicMonster::AttackCollisionBoxBeginOverlap);

		const FVector WorldPatrolPoint = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint);
		const FVector WorldPatrolPoint2 = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint2);
		DrawDebugSphere(GetWorld(), WorldPatrolPoint, 25.f, 12, FColor::Red, true);
		DrawDebugSphere(GetWorld(), WorldPatrolPoint2, 25.f, 12, FColor::Blue, true);

		BasicMonsterAIController = Cast<ABasicMonsterAIController>(GetController());

		if (BasicMonsterAIController)
		{
			BasicMonsterAIController->GetBlackboardComponent()->SetValueAsVector(TEXT("PatrolPoint"), WorldPatrolPoint);
			BasicMonsterAIController->GetBlackboardComponent()->SetValueAsVector(TEXT("PatrolPoint2"), WorldPatrolPoint2);
			BasicMonsterAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("CanAttack"), true);

			BasicMonsterAIController->RunBehaviorTree(BehaviorTree);
		}
	}

	if (HasAuthority())
	{ // Damage
		OnTakeAnyDamage.AddDynamic(this, &ABasicMonster::OnAttacked);
	}
}

void ABasicMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	
	UpdateHitDamages();
}

void ABasicMonster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABasicMonster::WeaponHit_Implementation(FHitResult HitResult)
{
	if (HitParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticles, HitResult.Location, FRotator(0.f), true);
	}
	ShowHpBar();
}

void ABasicMonster::OnAttacked(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{ // server only
	if (bDying) return;

	if (BasicMonsterAIController)
	{ // 피해입힌 적을 Target으로 계속 변경
		if (DamageCauser)
		{
			AWeapon* AttackerWeapon = Cast<AWeapon>(DamageCauser);
			if (AttackerWeapon)
			{
				BasicMonsterAIController->GetBlackboardComponent()->SetValueAsObject(FName("Target"), AttackerWeapon->GetOwner());
			}
		}
	}

	Hp = FMath::Clamp(Hp - Damage, 0.f, MaxHp);
	UE_LOG(LogTemp, Warning, TEXT("Hp : %f"), Hp);
	if (FMath::IsNearlyZero(Hp))
	{ // Death
		// TODO : GameMode, SpearmanCharacter->GameState?
		// TODO : Warning use Widget After Destory kinda Timer
		Death();
	}
	else
	{ // Hit
		// 랜덤 값에 따라 피격이 실행되는 경우는 랜덤 결과를 같이 보내거나 NetMulticast를 써서 동기화하는 수 밖에...
		const float Stunned = FMath::FRandRange(0.f, 1.f);
		if (Stunned <= StunChance)
		{
			MulticastHit();
			SetStunned(true);
		}
	}
}

void ABasicMonster::ShowHitDamage(int32 Damage, FVector HitLocation, bool bHeadShot)
{
	/*FVector2D HitDamagePosition;

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController)
	{
		UGameplayStatics::ProjectWorldToScreen(PlayerController, HitLocation, HitDamagePosition);
	}*/
	/*const float RandX = FMath::RandRange(HitDamagePosition.X - 100.f, HitDamagePosition.X + 100.f);
	const float RandY = FMath::RandRange(HitDamagePosition.Y - 100.f, HitDamagePosition.Y + 100.f);
	HitDamagePosition = FVector2D(RandX, RandY);*/

	HitDamageWidget->SetHitDamageText(Damage);
	HitDamageWidget->AddToViewport();
	//HitDamageWidget->SetPositionInViewport(HitDamagePosition);
	HitDamageWidget->PlayHitDamageAnimation(bHeadShot);
	StoreHitDamage(HitDamageWidget, HitLocation);
}

void ABasicMonster::StoreHitDamage(UHitDamageWidget* HitDamageToStore, FVector Location)
{
	HitDamages.Add(HitDamageToStore, Location);

	FTimerHandle HitDamageTimer;
	FTimerDelegate HitDamageDelegate;
	HitDamageDelegate.BindUFunction(this, FName("DestroyHitDamage"), HitDamageToStore);
	GetWorld()->GetTimerManager().SetTimer(HitDamageTimer, HitDamageDelegate, HitDamageDestroyTime, false);
}

void ABasicMonster::DestroyHitDamage(UHitDamageWidget* HitDamageToDestroy)
{
	HitDamages.Remove(HitDamageToDestroy);
	HitDamageToDestroy->RemoveFromParent();
}

void ABasicMonster::UpdateHitDamages()
{
	for (const auto& HitDamagePair : HitDamages)
	{
		UHitDamageWidget* HDWidget = HitDamagePair.Key;
		const FVector Location = HitDamagePair.Value;
		FVector2D ScreenPosition;
		UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), Location, ScreenPosition);
		HDWidget->SetPositionInViewport(ScreenPosition);
	}
}

void ABasicMonster::ShowHpBar()
{
	if (HpBar)
	{
		HpBar->SetVisibility(true);
	}
	GetWorldTimerManager().ClearTimer(HpBarTimer);
	GetWorldTimerManager().SetTimer(HpBarTimer, this, &ABasicMonster::HideHpBar, HpBarDisplayTime);
}

void ABasicMonster::HideHpBar()
{
	if (HpBar)
	{
		HpBar->SetVisibility(false);
	}
}

void ABasicMonster::Death()
{ //server only
	if (bDying) return;
	bDying = true;

	if (BasicMonsterAIController)
	{
		BasicMonsterAIController->GetBlackboardComponent()->SetValueAsBool(FName("Dead"), true);
		BasicMonsterAIController->StopMovement();
	}

	MulticastDeath();
	// TODO : many thing,
}

void ABasicMonster::AggroSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{ // server only
	if (OtherActor == nullptr) return;

	ASpearmanCharacter* SpearmanCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (SpearmanCharacter)
	{
		BasicMonsterAIController->GetBlackboardComponent()->SetValueAsObject(TEXT("Target"), SpearmanCharacter);
		BasicMonsterAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("CharacterDead"), false);
	}
}

void ABasicMonster::CombatRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{ // server only
	if (OtherActor == nullptr) return;
	ASpearmanCharacter* SpearmanCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (SpearmanCharacter)
	{
		bInCombatRange = true;
		if (BasicMonsterAIController)
		{
			BasicMonsterAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("InCombatRange"), true);
		}
	}
}

void ABasicMonster::CombatRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{ // server only
	if (OtherActor == nullptr) return;
	ASpearmanCharacter* SpearmanCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (SpearmanCharacter)
	{
		bInCombatRange = false;
		if (BasicMonsterAIController)
		{
			BasicMonsterAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("InCombatRange"), false);
		}
	}
}

void ABasicMonster::AttackCollisionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{ // server only
	if (OtherActor == nullptr || OtherActor == this) return;
	if (HitSet.Contains(OtherActor)) return;

	HitSet.Add(OtherActor);
	ASpearmanCharacter* SpearmanCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (SpearmanCharacter)
	{
		UGameplayStatics::ApplyDamage(SpearmanCharacter, BaseDamage, BasicMonsterAIController, this, UDamageType::StaticClass());
	}
}

void ABasicMonster::SetCanAttack()
{ // server only
	bCanAttack = true;
	if (BasicMonsterAIController)
	{
		BasicMonsterAIController->GetBlackboardComponent()->SetValueAsBool(FName("CanAttack"), true);
	}
}

void ABasicMonster::SetCanAttackTimer()
{ // server only, called in BTTask_Attack after MulticastPlayAttackMontage()
	bCanAttack = false;
	GetWorldTimerManager().SetTimer(AttackWaitTimer, this, &ABasicMonster::SetCanAttack, AttackWaitTime);
	if (BasicMonsterAIController)
	{
		BasicMonsterAIController->GetBlackboardComponent()->SetValueAsBool(FName("CanAttack"), false);
	}
}

void ABasicMonster::TurnOnAttackCollision()
{
	HitSet.Empty();
	AttackCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void ABasicMonster::TurnOffAttackCollision()
{
	AttackCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitSet.Empty();
}

void ABasicMonster::SetStunned(bool Stunned)
{
	if (HasAuthority())
	{
		bStunned = Stunned;

		if (BasicMonsterAIController)
		{
			BasicMonsterAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("Stunned"), Stunned);
		}
	}
}

void ABasicMonster::MulticastHit_Implementation()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitMontage)
	{
		AnimInstance->Montage_Play(HitMontage);
		AnimInstance->Montage_JumpToSection(FName("Hit"), HitMontage);
	}
}

void ABasicMonster::MulticastAttack_Implementation()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && AttackMontage)
	{
		AnimInstance->Montage_Play(AttackMontage);
		AnimInstance->Montage_JumpToSection(FName("Attack"), AttackMontage);
	}
}

void ABasicMonster::MulticastDeath_Implementation()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);
		AnimInstance->Montage_JumpToSection(FName("Death"), DeathMontage);
	}
}

void ABasicMonster::OnRep_Hp()
{ // client only
	HpBarWidget->SetHpBar(GetHpRatio());
	if (FMath::IsNearlyZero(Hp))
	{
		//Die();
	}
}