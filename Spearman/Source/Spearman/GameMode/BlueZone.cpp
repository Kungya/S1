// Fill out your copyright notice in the Description page of Project Settings.


#include "BlueZone.h"
#include "Components/CapsuleComponent.h"
#include "Spearman/Character/SpearmanCharacter.h"

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

	if (HasAuthority())
	{
		FBlueZoneInfo BlueZoneInfoPhase0 = { 5.f, 10.f, 1.f };
		BlueZoneInfoArray.Add(BlueZoneInfoPhase0);

		FBlueZoneInfo BlueZoneInfoPhase1 = { 5.f, 5.f, 0.5f };
		BlueZoneInfoArray.Add(BlueZoneInfoPhase1);

		FBlueZoneInfo BlueZoneInfoPhase2 = { 5.f, 5.f, 0.5f };
		BlueZoneInfoArray.Add(BlueZoneInfoPhase2);

		ZoneMesh->OnComponentBeginOverlap.AddDynamic(this, &ABlueZone::OnBlueZoneBeginOverlap);
		ZoneMesh->OnComponentEndOverlap.AddDynamic(this, &ABlueZone::OnBlueZoneEndOverlap);
	}
}

void ABlueZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABlueZone::OnBlueZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{ /* Server Only */
	// UE_LOG(LogTemp, Warning, TEXT("Begin Overlap"));

	ASpearmanCharacter* OverlappedSpearmanCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (OverlappedSpearmanCharacter)
	{
		OverlappedSpearmanCharacter->SetbIsInBlueZone(true);
	}
}

void ABlueZone::OnBlueZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{ /* Server Only */
	// UE_LOG(LogTemp, Warning, TEXT("End Overlap"));

	ASpearmanCharacter* OverlappedSpearmanCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (OverlappedSpearmanCharacter)
	{
		OverlappedSpearmanCharacter->SetbIsInBlueZone(false);
	}
}

void ABlueZone::StartMovingBlueZone()
{ /* Server Only */
	if (CurrentPhase < BlueZoneInfoArray.Num())
	{
		// Start reducing the BlueZone
		GetWorld()->GetTimerManager().SetTimer(MovingTimerHandle, this, &ABlueZone::ReduceBlueZone, 0.2, true);
		
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
		UE_LOG(LogTemp, Warning, TEXT("Overall Phase is Over"));
	}
}

void ABlueZone::ReduceBlueZone()
{ /* Server Only */
	const FVector ScaleToReduce = GetActorScale3D() - FVector(BlueZoneInfoArray[CurrentPhase].ScaleToDecrease, BlueZoneInfoArray[CurrentPhase].ScaleToDecrease, 0.f);
	// UE_LOG(LogTemp, Warning, TEXT("Current Phase : %d, Current Scale : %s"), CurrentPhase, *ScaleToReduce.ToString());
	SetActorScale3D(ScaleToReduce);
}

void ABlueZone::StopBlueZone()
{ /* Server Only */
	GetWorld()->GetTimerManager().ClearTimer(MovingTimerHandle);

	if (CurrentPhase < BlueZoneInfoArray.Num() - 1)
	{
		CurrentPhase++;
	}
	else
	{
		Destroy();
	}
}
