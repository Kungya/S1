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
#include "Spearman/Items/Item.h"

ABasicMonster::ABasicMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	HitDamageWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HitDamageWidget"));
	HitDamageWidget->SetupAttachment(GetMesh());

	HpBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBarWidget"));
	HpBarWidget->SetupAttachment(GetMesh());
	HpBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HpBarWidget->SetDrawSize(FVector2D(125.f, 20.f));

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

	/*
	* Hit Box, WARNING : Never RELOCATE HitBox's Order, "HitBoxArray[0] => head, HitBoxArray[1] => nose"
	*/

	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("PIG_-Head"));
	head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	head->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	HitBoxArray.Add(head);

	nose = CreateDefaultSubobject<UBoxComponent>(TEXT("nose"));
	nose->SetupAttachment(GetMesh(), FName("Pig_Nose"));
	nose->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(nose);

	neck = CreateDefaultSubobject<UBoxComponent>(TEXT("neck"));
	neck->SetupAttachment(GetMesh(), FName("PIG_-Neck"));
	neck->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(neck);

	spine = CreateDefaultSubobject<UBoxComponent>(TEXT("spine"));
	spine->SetupAttachment(GetMesh(), FName("PIG_-Spine"));
	spine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(spine);

	spine_1 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_1"));
	spine_1->SetupAttachment(GetMesh(), FName("PIG_-Spine1"));
	spine_1->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(spine_1);

	forearm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("forearm_l"));
	forearm_l->SetupAttachment(GetMesh(), FName("PIG_-L-Forearm"));
	forearm_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(forearm_l);

	forearm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("forearm_r"));
	forearm_r->SetupAttachment(GetMesh(), FName("PIG_-R-Forearm"));
	forearm_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(forearm_r);

	horselink_l = CreateDefaultSubobject<UBoxComponent>(TEXT("horselink_l"));
	horselink_l->SetupAttachment(GetMesh(), FName("PIG_-L-HorseLink"));
	horselink_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(horselink_l);

	horselink_r = CreateDefaultSubobject<UBoxComponent>(TEXT("horselink_r"));
	horselink_r->SetupAttachment(GetMesh(), FName("PIG_-R-HorseLink"));
	horselink_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxArray.Add(horselink_r);

}

void ABasicMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABasicMonster, Hp);
}

void ABasicMonster::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	HitDamageWidget->InitWidget();
	HitDamage = Cast<UHitDamageWidget>(HitDamageWidget->GetUserWidgetObject());

	HpBarWidget->InitWidget();
	HpBar = Cast<UHpBarWidget>(HpBarWidget->GetUserWidgetObject());
}

void ABasicMonster::BeginPlay()
{
	Super::BeginPlay();

	HitDamageWidget->SetVisibility(false);

	HpBar->SetHpBar(GetHpRatio());
	HpBar->SetVisibility(ESlateVisibility::Hidden);

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABasicMonster::OnAttacked);

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

void ABasicMonster::WeaponHit(int32 Damage, FVector_NetQuantize HitPoint, bool bHeadShot)
{
	if (HitParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticles, HitPoint, FRotator(0.f), true);
	}
	ShowHpBar();
	ShowHitDamage(Damage, HitPoint, bHeadShot);
}

void ABasicMonster::OnAttacked(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{ /* Server Only */
	if (bDying) return;

	if (BasicMonsterAIController)
	{ // Set Target Attacker
		if (DamageCauser)
		{
			BasicMonsterAIController->GetBlackboardComponent()->SetValueAsObject(FName("Target"), DamageCauser->GetOwner());
		}
	}

	Hp = FMath::Clamp(Hp - Damage, 0.f, MaxHp);
	if (FMath::IsNearlyZero(Hp))
	{
		Death();
	}
	else
	{
		const float Stunned = FMath::FRandRange(0.f, 1.f);
		if (Stunned <= StunChance)
		{
			MulticastHit();
			SetStunned(true);
		}
	}
}

void ABasicMonster::ShowHitDamage(int32 Damage, FVector_NetQuantize HitLocation, bool bHeadShot)
{
	HitDamage->SetHitDamageText(Damage);
	HitDamage->AddToViewport();
	HitDamage->PlayHitDamageAnimation(bHeadShot);
	StoreHitDamage(HitDamage, HitLocation);
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
		UHitDamageWidget* InHitDamage = HitDamagePair.Key;
		const FVector Location = HitDamagePair.Value;
		FVector2D ScreenPosition;
		UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), Location, ScreenPosition);
		InHitDamage->SetPositionInViewport(ScreenPosition);
	}
}

void ABasicMonster::ShowHpBar()
{
	if (HpBar)
	{
		HpBar->SetVisibility(ESlateVisibility::Visible);
	}
	GetWorldTimerManager().ClearTimer(HpBarTimer);
	GetWorldTimerManager().SetTimer(HpBarTimer, this, &ABasicMonster::HideHpBar, HpBarDisplayTime);
}

void ABasicMonster::HideHpBar()
{
	if (HpBar)
	{
		HpBar->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ABasicMonster::Death()
{ /* Server Only */
	if (bDying) return;
	bDying = true;

	if (BasicMonsterAIController)
	{
		BasicMonsterAIController->GetBlackboardComponent()->SetValueAsBool(FName("Dead"), true);
		BasicMonsterAIController->StopMovement();
	}

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DropItems();
	MulticastDeath();
}

void ABasicMonster::DropItems()
{ /* Server Only */
	const int32 RandomNum = FMath::RandRange(1, 5);

	AItem* ItemToSpawn = GetWorld()->SpawnActorDeferred<AItem>(ItemClass, GetActorTransform());
	if (ItemToSpawn)
	{
		ItemToSpawn->Init(RandomNum);
		ItemToSpawn->FinishSpawning(GetActorTransform());
	}
}

void ABasicMonster::AggroSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{ /* Server Only */
	if (OtherActor == nullptr) return;

	ASpearmanCharacter* SpearmanCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (SpearmanCharacter)
	{
		BasicMonsterAIController->GetBlackboardComponent()->SetValueAsObject(TEXT("Target"), SpearmanCharacter);
		BasicMonsterAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("CharacterDead"), false);
	}
}

void ABasicMonster::CombatRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{ /* Server Only */
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
{ /* Server Only */
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
{ /* Server Only */
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
{ /* Server Only */
	bCanAttack = true;
	if (BasicMonsterAIController)
	{
		BasicMonsterAIController->GetBlackboardComponent()->SetValueAsBool(FName("CanAttack"), true);
	}
}

void ABasicMonster::SetCanAttackTimer()
{ /* Server Only, Called in BTTask_Attack after MulticastPlayAttackMontage() */
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
{ /* Server Only, Called in OnAttacked() */
	bStunned = Stunned;

	if (BasicMonsterAIController)
	{
		BasicMonsterAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("Stunned"), Stunned);
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

void ABasicMonster::OnRep_Hp(float LastHp)
{ // Client Only
	HpBar->SetHpBar(GetHpRatio());
	if (FMath::IsNearlyZero(Hp))
	{
		//Die();
	}
}