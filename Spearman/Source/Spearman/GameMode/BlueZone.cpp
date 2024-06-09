// Fill out your copyright notice in the Description page of Project Settings.


#include "BlueZone.h"
#include "Components/CapsuleComponent.h"

ABlueZone::ABlueZone()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	ZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneMesh"));
	ZoneMesh->SetIsReplicated(true);
	SetRootComponent(ZoneMesh);

	// For testing : Character should be able to see Bluezone in Everywhere
	// bAlwaysRelevant = true;
}

void ABlueZone::BeginPlay()
{
	Super::BeginPlay();

	FBlueZoneInfo BlueZoneInfoPhase0 = { 5.f, 5.f, 1.f };
	BlueZoneInfoArray.Add(BlueZoneInfoPhase0);

	FBlueZoneInfo BlueZoneInfoPhase1 = { 5.f, 15.f, 0.01f };
	BlueZoneInfoArray.Add(BlueZoneInfoPhase1);
}

void ABlueZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABlueZone::StartMovingBlueZone()
{ /* Server Only */
	if (CurrentPhase < BlueZoneInfoArray.Num())
	{
		// Start reducing the BlueZone
		GetWorld()->GetTimerManager().SetTimer(MovingTimerHandle, this, &ABlueZone::ReduceBlueZone, 0.05, true);
		
		// Stop Reducing BlueZone if moving time is reached
		FTimerHandle LocalHandle1;
		GetWorld()->GetTimerManager().SetTimer(LocalHandle1, this, &ABlueZone::StopBlueZone, BlueZoneInfoArray[CurrentPhase].MovingTime, false);
		
		const float TotalTimeForNextPhase = BlueZoneInfoArray[CurrentPhase].WaitingTime + BlueZoneInfoArray[CurrentPhase].MovingTime;
		
		// Recursive call after TotalTimeForNextPhase
		FTimerHandle LocalHandle2;
		GetWorld()->GetTimerManager().SetTimer(LocalHandle2, this, &ABlueZone::StartMovingBlueZone, TotalTimeForNextPhase, false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Current Phase is Over"));
	}
}

void ABlueZone::ReduceBlueZone()
{
	const FVector ScaleToReduce = GetActorScale3D() - FVector(BlueZoneInfoArray[CurrentPhase].ScaleToDecrease, BlueZoneInfoArray[CurrentPhase].ScaleToDecrease, 0.f);
	UE_LOG(LogTemp, Warning, TEXT("Current Scale : %s"), *ScaleToReduce.ToString());
	SetActorScale3D(ScaleToReduce);
}

void ABlueZone::StopBlueZone()
{
	if (CurrentPhase < BlueZoneInfoArray.Num() - 1)
	{
		CurrentPhase++;
	}

	GetWorld()->GetTimerManager().ClearTimer(MovingTimerHandle);
	MovingTimerHandle.Invalidate();
}
