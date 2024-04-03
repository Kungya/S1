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

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

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
	TraceStartBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TraceEndBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TraceEndBox"));
	TraceEndBox->SetupAttachment(RootComponent);
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
	
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}

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
	Params.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceMultiByChannel(HitResults, Start, End, ECC_Visibility, Params))
	{ // hit true

		for (const FHitResult& HitResult : HitResults)
		{
			if (!HitSet.Contains(HitResult.GetActor()))
			{
				if (HitResult.GetActor())
				{
					HitSet.Add(HitResult.GetActor());

					IWeaponHitInterface* WeaponHitInterface = Cast<IWeaponHitInterface>(HitResult.GetActor());
					if (WeaponHitInterface)
					{ // Cast will succeed if BasicMonster, since BasicMonster inherits from WeaponHitInterface
						WeaponHitInterface->WeaponHit_Implementation(HitResult);
					}
					else
					{ // TODO : 여기로 들어옴
						UE_LOG(LogTemp, Warning, TEXT("Not inherit Interface"));
					}

					if (HasAuthority())
					{ // server only
						OwnerCharacter = OwnerCharacter == nullptr ? Cast<ACharacter>(GetOwner()) : OwnerCharacter;
						if (OwnerCharacter)
						{
							OwnerController = OwnerController == nullptr ? OwnerCharacter->GetController() : OwnerController;
							if (OwnerController)
							{
								const float Dist = FVector::Distance(OwnerCharacter->GetActorLocation(), HitResult.GetActor()->GetActorLocation());
								// Dist (60, 240) -> Damage (10, 30)
								FVector2D InRange(60.f, 240.f);
								FVector2D OutRange(Damage / 3.f, Damage);
								const float Dmg = FMath::GetMappedRangeValueClamped(InRange, OutRange, Dist);

								UGameplayStatics::ApplyDamage(HitResult.GetActor(), FMath::RoundToFloat(Dmg), OwnerController, this, UDamageType::StaticClass());
							}
						}
					}
				}
			}
		}
	}

	// DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 5.f, 0, 5.f);
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		// 장착 후 충돌 off
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
	bAttackCollisionTrace = true;
}

void AWeapon::TurnOffAttackCollision()
{
	bAttackCollisionTrace = false;
	HitSet.Empty();
}
