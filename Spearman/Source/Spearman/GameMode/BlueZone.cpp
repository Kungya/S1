// Fill out your copyright notice in the Description page of Project Settings.


#include "BlueZone.h"
#include "Components/CapsuleComponent.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "ExtractionPoint.h"

ABlueZone::ABlueZone()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	ZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneMesh"));
	ZoneMesh->SetIsReplicated(true);
	SetRootComponent(ZoneMesh);
}

void ABlueZone::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{ // BlueZone Information for phase
		// { WaitingTime, MovingTime, ScaleToDecreasePerLoop }, Reduction Amount per Pahse : MovingTime * ScaleToDecreasePerLoop
		FBlueZoneInfo BlueZoneInfoPhase0 = { 1.f, 1.f, 3.f };
		BlueZoneInfoArray.Add(BlueZoneInfoPhase0);

		FBlueZoneInfo BlueZoneInfoPhase1 = { 1.f, 1.f, 2.f };
		BlueZoneInfoArray.Add(BlueZoneInfoPhase1);

		FBlueZoneInfo BlueZoneInfoPhase2 = { 1.f, 1.f, 2.f };
		BlueZoneInfoArray.Add(BlueZoneInfoPhase2);

		FBlueZoneInfo BlueZoneInfoPhase3 = { 1.f, 1.f, 2.f };
		BlueZoneInfoArray.Add(BlueZoneInfoPhase3);

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
	ASpearmanCharacter* OverlappedSpearmanCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (OverlappedSpearmanCharacter)
	{
		OverlappedSpearmanCharacter->SetbIsInBlueZone(true);
		OverlappedSpearmanCharacter->ShowBlueZoneImage();
	}
}

void ABlueZone::OnBlueZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{ /* Server Only */
	ASpearmanCharacter* OverlappedSpearmanCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (OverlappedSpearmanCharacter)
	{
		OverlappedSpearmanCharacter->SetbIsInBlueZone(false);
		OverlappedSpearmanCharacter->ShowBlueZoneImage();
	}
}

void ABlueZone::StartMovingBlueZone()
{ /* Server Only */
	if (CurrentPhase < BlueZoneInfoArray.Num() - 1)
	{
		// Start reducing the BlueZone (Loop) until MovingTime
		GetWorld()->GetTimerManager().SetTimer(MovingTimerHandle, this, &ABlueZone::ReduceBlueZone, 0.2, true);
		
		// Stop reducing BlueZone if MovingTime is reached
		FTimerHandle StopHandle;
		GetWorld()->GetTimerManager().SetTimer(StopHandle, this, &ABlueZone::StopBlueZone, BlueZoneInfoArray[CurrentPhase].MovingTime, false);
		
		const float TotalTimeForNextPhase = BlueZoneInfoArray[CurrentPhase].WaitingTime + BlueZoneInfoArray[CurrentPhase].MovingTime;
		FTimerHandle RecursiveHandle;
		GetWorld()->GetTimerManager().SetTimer(RecursiveHandle, this, &ABlueZone::StartMovingBlueZone, TotalTimeForNextPhase, false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Overall Phase is Over"));

		FTransform Transform(FRotator(), FVector(GetActorLocation().X, GetActorLocation().Y, 10.f));
		AExtractionPoint* ExtractionPoint = GetWorld()->SpawnActor<AExtractionPoint>(ExtractionPointClass, Transform);
		if (ExtractionPoint)
		{
			UE_LOG(LogTemp, Warning, TEXT("Extraction Point is Spawned"));
		}
	}
}

void ABlueZone::ReduceBlueZone()
{ /* Server Only */
	const FVector ReducedScale = GetActorScale3D() - FVector(BlueZoneInfoArray[CurrentPhase].ScaleToDecreasePerLoop, BlueZoneInfoArray[CurrentPhase].ScaleToDecreasePerLoop, 0.f);
	SetActorScale3D(ReducedScale);

	if (CurrentPhase >= 1)
	{ // Move
		const FVector NewLocation(GetActorLocation() + NormalizedRandomVector * 50.f);
		SetActorLocation(NewLocation);
	}
}

void ABlueZone::StopBlueZone()
{ /* Server Only, Prepare next Phase, next Phase gonna start after WaitingTime */
	GetWorld()->GetTimerManager().ClearTimer(MovingTimerHandle);

	if (CurrentPhase < BlueZoneInfoArray.Num() - 1)
	{ // To Next Phase
		CurrentPhase++;

		CalcMoveVector();
	}
	else
	{ // End
		UE_LOG(LogTemp, Warning, TEXT("x : %f, y : %f"), GetActorScale().X, GetActorScale().Y);
	}
}

void ABlueZone::CalcMoveVector()
{
	FVector RandomizedVector(FMath::RandRange(-1.f, 1.f), FMath::RandRange(-1.f, 1.f), 0.f);
	NormalizedRandomVector = RandomizedVector.GetSafeNormal();
}
