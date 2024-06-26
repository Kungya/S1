// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Spearman/Spearman.h"
#include "Kismet/GameplayStatics.h"
#include "Spearman/Interfaces/WeaponHitInterface.h"
#include "Spearman/Monster/BasicMonster.h"
#include "Spearman/SpearComponents/LagCompensationComponent.h"
#include "DrawDebugHelpers.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);

	TraceStartBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TraceStartBox"));
	TraceStartBox->SetupAttachment(RootComponent);
	TraceStartBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	TraceStartBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TraceEndBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TraceEndBox"));
	TraceEndBox->SetupAttachment(RootComponent);
	TraceEndBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	TraceEndBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME_CONDITION(AWeapon, bAttackCollisionTrace, COND_OwnerOnly);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	// triggered for show widget, called in both server and client
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);

	ShowPickupWidget(false);
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ASpearmanCharacter* SpearmanCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (SpearmanCharacter)
	{
		SpearmanCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ASpearmanCharacter* SpearmanCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (SpearmanCharacter)
	{
		SpearmanCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// TODO : Refactor to AnimState
	// if (bUseReiwnd) : Client, else : Server */
	if (bAttackCollisionTrace)
	{
		if (bUseRewind && !HasAuthority())
		{
			// UE_LOG(LogTemp, Warning, TEXT(" bUseRewind in Tick, Client"))
			AttackCollisionCheckByRewind();
		}
		else if (!bUseRewind && HasAuthority())
		{
			// UE_LOG(LogTemp, Warning, TEXT(" !bUseRewind in Tick, Server"))
			AttackCollisionCheckByServer();
		}
	}
}

void AWeapon::AttackCollisionCheckByRewind()
{ /* Client Only, Simulate In Client First */
	FVector Start = TraceStartBox->GetComponentLocation();
	FVector End = TraceEndBox->GetComponentLocation();
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GetOwner());

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.f);

	if (HitResult.GetActor() == nullptr || HitSet.Contains(HitResult.GetActor())) return;
	HitSet.Add(HitResult.GetActor());
	
	OwnerSpearmanCharacter = (OwnerSpearmanCharacter == nullptr) ? Cast<ASpearmanCharacter>(GetOwner()) : OwnerSpearmanCharacter;
	OwnerSpearmanPlayerController = (OwnerSpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(OwnerSpearmanCharacter->GetController()) : OwnerSpearmanPlayerController;
	if (OwnerSpearmanCharacter == nullptr || OwnerSpearmanPlayerController == nullptr) return;

	ASpearmanCharacter* HitSpearmanCharacter = Cast<ASpearmanCharacter>(HitResult.GetActor());
	if (HitSpearmanCharacter)
	{ /* Hit SpearmanCharacter */
		UE_LOG(LogTemp, Warning, TEXT("Hit in Client"));
		if (OwnerSpearmanPlayerController && OwnerSpearmanCharacter && OwnerSpearmanCharacter->GetLagCompensation())
		{
			const float CurrentClientTime = OwnerSpearmanPlayerController->GetServerTime() - OwnerSpearmanPlayerController->GetSingleTripTime();
			OwnerSpearmanCharacter->GetLagCompensation()->ServerRewindRequest(HitSpearmanCharacter, Start, HitResult.ImpactPoint, CurrentClientTime, this);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("LagCompensation Component nullptr"));
		}
	}

	/* TODO : double check about Hit Dupplication in client */
	/* TODO : Rewind BasicMonster, all Actors that can Take Damage */
}

void AWeapon::AttackCollisionCheckByServer()
{ /* Server Only */
	FVector Start = TraceStartBox->GetComponentLocation();
	FVector End = TraceEndBox->GetComponentLocation();
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GetOwner());
	
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.f);

	if (HitResult.GetActor() == nullptr || HitSet.Contains(HitResult.GetActor())) return;
	HitSet.Add(HitResult.GetActor());

	OwnerSpearmanCharacter = (OwnerSpearmanCharacter == nullptr) ? Cast<ASpearmanCharacter>(GetOwner()) : OwnerSpearmanCharacter;
	if (OwnerSpearmanCharacter == nullptr) return;
	OwnerSpearmanPlayerController = (OwnerSpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(OwnerSpearmanCharacter->GetController()) : OwnerSpearmanPlayerController;
	if (OwnerSpearmanPlayerController == nullptr) return;
	
	bool bHeadShot = false;
	ASpearmanCharacter* HitSpearmanCharacter = Cast<ASpearmanCharacter>(HitResult.GetActor());
	if (HitSpearmanCharacter)
	{ /* Hit SpearmanCharacter */
		if (HitResult.BoneName.ToString() == HitSpearmanCharacter->GetHeadBone())
		{
			bHeadShot = true;
			HitPartDamage = HeadShotDamage;
		}
		else
		{
			bHeadShot = true;
			HitPartDamage = Damage;
		}
		const float Dist = FVector::Distance(OwnerSpearmanCharacter->GetActorLocation(), HitResult.GetActor()->GetActorLocation());
		FVector2D InRange(60.f, 240.f);
		FVector2D OutRange(HitPartDamage / 3.f, HitPartDamage);
		const float InDamage = FMath::GetMappedRangeValueClamped(InRange, OutRange, Dist);
		UGameplayStatics::ApplyDamage(HitResult.GetActor(), FMath::RoundToFloat(InDamage), OwnerSpearmanPlayerController, this, UDamageType::StaticClass());
		
		MulticastHit(HitResult.GetActor(), FMath::CeilToInt(InDamage), HitResult.ImpactPoint, bHeadShot);
	}
	ABasicMonster* HitMonster = Cast<ABasicMonster>(HitResult.GetActor());
	if (HitMonster)
	{ /* Hit BasicMonster */
		if (HitResult.BoneName.ToString() == HitMonster->GetHeadBone())
		{
			bHeadShot = true;
			HitPartDamage = HeadShotDamage;
		}
		else
		{
			bHeadShot = false;
			HitPartDamage = Damage;
		}
		const float Dist = FVector::Distance(OwnerSpearmanCharacter->GetActorLocation(), HitResult.GetActor()->GetActorLocation());
		FVector2D InRange(60.f, 240.f);
		FVector2D OutRange(HitPartDamage / 3.f, HitPartDamage);
		const float InDamage = FMath::GetMappedRangeValueClamped(InRange, OutRange, Dist);
		
		UGameplayStatics::ApplyDamage(HitResult.GetActor(), FMath::RoundToFloat(InDamage), OwnerSpearmanPlayerController, this, UDamageType::StaticClass());
		MulticastHit(HitResult.GetActor(), FMath::FloorToInt(InDamage), HitResult.ImpactPoint, bHeadShot);
	}
}

void AWeapon::MulticastHit_Implementation(AActor* HitActor, int32 InDamage, FVector_NetQuantize HitPoint, bool bHeadShot)
{ /* Unreliable */
	IWeaponHitInterface* WeaponHitInterface = Cast<IWeaponHitInterface>(HitActor);
	if (WeaponHitInterface)
	{ // Cast will succeed if BasicMonster, since BasicMonster inherits from WeaponHitInterface
		WeaponHitInterface->WeaponHit_Implementation(InDamage, HitPoint, bHeadShot);
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		// 장착후 상호작용 off
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		if (HasAuthority())
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
		case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		case EWeaponState::EWS_Dropped:
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
}

void AWeapon::ShowPickupWidget(bool bShowPickupWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowPickupWidget);
	}
}

void AWeapon::Dropped()
{ /* Server Only, Called in SpearmanCharacter::Death() */
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
}

void AWeapon::TurnOnAttackCollision()
{ 
	if (bUseRewind && !HasAuthority())
	{ /* Client Only */
		HitSet.Empty();
	}
	else if (!bUseRewind && HasAuthority())
	{ /* Server Only */
		HitSet.Empty();
	}

	if (HasAuthority())
	{
		bAttackCollisionTrace = true;
	}
}

void AWeapon::TurnOffAttackCollision()
{ /* Server Only */
	
	if (bUseRewind && !HasAuthority())
	{ /* Client Only */
		HitSet.Empty();
	}
	else if (!bUseRewind && HasAuthority())
	{ /* Server Only */
		HitSet.Empty();
	}

	if (HasAuthority())
	{
		bAttackCollisionTrace = false;
	}
}