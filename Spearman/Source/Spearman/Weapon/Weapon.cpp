// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Spearman/Spearman.h"
#include "Kismet/GameplayStatics.h"
#include "Spearman/Interfaces/WeaponHitInterface.h"
#include "Spearman/Monster/BasicMonster.h"

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

	if (bAttackCollisionTrace)
	{
		AttackCollisionCheckByTrace();
	}
}

void AWeapon::AttackCollisionCheckByTrace()
{ // client and server
	FVector Start = TraceStartBox->GetComponentLocation();
	FVector End = TraceEndBox->GetComponentLocation();
	TArray<FHitResult> HitResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GetOwner());
	
	GetWorld()->LineTraceMultiByChannel(HitResults, Start, End, ECC_Visibility, Params);
	if (HitResults.Num() == 0) return;

	for (const FHitResult& HitResult : HitResults)
	{
		if (HitResult.GetActor() == nullptr) continue;

		if (!HitSet.Contains(HitResult.GetActor()))
		{
			HitSet.Add(HitResult.GetActor());
			IWeaponHitInterface* WeaponHitInterface = Cast<IWeaponHitInterface>(HitResult.GetActor());
			if (WeaponHitInterface)
			{ // Cast will succeed if BasicMonster, since BasicMonster inherits from WeaponHitInterface
				WeaponHitInterface->WeaponHit_Implementation(HitResult);
			}
			OwnerCharacter = (OwnerCharacter == nullptr) ? Cast<ACharacter>(GetOwner()) : OwnerCharacter;
			if (OwnerCharacter)
			{
				OwnerController = (OwnerController == nullptr) ? OwnerCharacter->GetController() : OwnerController;
				if (OwnerController)
				{
					if (HitResult.GetActor()->IsA<ASpearmanCharacter>())
					{ // Hit SpearmanCharacter
						ASpearmanCharacter* HitSpearmanCharacter = Cast<ASpearmanCharacter>(HitResult.GetActor());
						if (HitSpearmanCharacter)
						{
							if (HitResult.BoneName.ToString() == HitSpearmanCharacter->GetHeadBone())
							{ // Head Damage
								FinalDamage = HeadShotDamage;
							}
							else
							{ // Body Damage
								FinalDamage = Damage;
							}
							UE_LOG(LogTemp, Warning, TEXT("Hit Spearman Component : %s"), *HitResult.BoneName.ToString());
							const float Dist = FVector::Distance(OwnerCharacter->GetActorLocation(), HitResult.GetActor()->GetActorLocation());
							FVector2D InRange(60.f, 240.f);
							FVector2D OutRange(FinalDamage / 3.f, FinalDamage);
							const float InDamage = FMath::GetMappedRangeValueClamped(InRange, OutRange, Dist);
							UGameplayStatics::ApplyDamage(HitResult.GetActor(), FMath::RoundToFloat(InDamage), OwnerController, this, UDamageType::StaticClass());
							// TODO : HitDamage Widget, HitSpearmanCharacter->ShowHitDamage();
						}
					}
					else if (HitResult.GetActor()->IsA<ABasicMonster>())
					{ // Hit BasicMonster
						ABasicMonster* HitMonster = Cast<ABasicMonster>(HitResult.GetActor());
						if (HitMonster)
						{
							bool bHeadShot = false;
							if (HitResult.BoneName.ToString() == HitMonster->GetHeadBone())
							{ // Head Damage
								bHeadShot = true;
								FinalDamage = HeadShotDamage;
							}
							else
							{ // Body Damage
								bHeadShot = false;
								FinalDamage = Damage;
							}
							const float Dist = FVector::Distance(OwnerCharacter->GetActorLocation(), HitResult.GetActor()->GetActorLocation());
							FVector2D InRange(60.f, 240.f);
							FVector2D OutRange(FinalDamage / 3.f, FinalDamage);
							const float InDamage = FMath::GetMappedRangeValueClamped(InRange, OutRange, Dist);
							if (HasAuthority())
							{
								UGameplayStatics::ApplyDamage(HitResult.GetActor(), FMath::RoundToFloat(InDamage), OwnerController, this, UDamageType::StaticClass());
							}
							if (OwnerCharacter->IsLocallyControlled())
							{ // HitDamage Widget
								HitMonster->ShowHitDamage(InDamage, HitResult.Location, bHeadShot);
							}
						}
					}
				}
			}
		}
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
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
}

void AWeapon::TurnOnAttackCollision()
{
	HitSet.Empty();
	bAttackCollisionTrace = true;
}

void AWeapon::TurnOffAttackCollision()
{
	HitSet.Empty();
	bAttackCollisionTrace = false;
}
