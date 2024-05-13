// Fill out your copyright notice in the Description page of Project Settings.


#include "BlueZone.h"
#include "Components/CapsuleComponent.h"

ABlueZone::ABlueZone()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	SetRootComponent(Scene);

	SafeZone = CreateDefaultSubobject<UCapsuleComponent>(TEXT("SafeZone"));
	SafeZone->SetupAttachment(Scene);
	SafeZone->SetCapsuleHalfHeight(DefaultCapsuleHalfHeight);
	SafeZone->SetCapsuleRadius(DefaultCapsuleRadius);

	SafeZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SafeZoneMesh"));
	SafeZoneMesh->SetupAttachment(Scene);

	BlueZone = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BlueZone"));
	BlueZone->SetupAttachment(Scene);
	BlueZone->SetCapsuleHalfHeight(DefaultCapsuleHalfHeight);
	BlueZone->SetCapsuleRadius(DefaultCapsuleRadius);
}


void ABlueZone::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		BlueZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BlueZone->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		BlueZone->OnComponentBeginOverlap.AddDynamic(this, &ABlueZone::OnBlueZoneBeginOverlap);
		
		SafeZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SafeZone->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		SafeZone->OnComponentBeginOverlap.AddDynamic(this, &ABlueZone::OnSafeZoneBeginOverlap);
		SafeZone->OnComponentEndOverlap.AddDynamic(this, &ABlueZone::OnSafeZoneEndOverlap);
	}
	
}

void ABlueZone::OnBlueZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

}

void ABlueZone::OnSafeZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

}

void ABlueZone::OnSafeZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{

}

void ABlueZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}