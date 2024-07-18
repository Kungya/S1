// Fill out your copyright notice in the Description page of Project Settings.


#include "ExtractionPoint.h"
#include "Components/SphereComponent.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"

AExtractionPoint::AExtractionPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneComponent);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(RootComponent);

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(RootComponent);

}

void AExtractionPoint::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &AExtractionPoint::OnSphereBeginOverlap);
		OverlapSphere->OnComponentEndOverlap.AddDynamic(this, &AExtractionPoint::OnSphereEndOverlap);
	}
}

void AExtractionPoint::Extraction()
{ /* Server Only */
	UE_LOG(LogTemp, Warning, TEXT("Extraction !!"));


	if (CharacterToExtract)
	{
		
		ASpearmanPlayerController* SpearmanPlayerController = Cast<ASpearmanPlayerController>(CharacterToExtract->Controller);
		if (SpearmanPlayerController)
		{
			SpearmanPlayerController->ClientReturnToMainMenuWithTextReason(FText());

			// TODO : Save Inventroy TArray with Unique Id
		}
	}
}

void AExtractionPoint::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{ /* Server Only */
	UE_LOG(LogTemp, Warning, TEXT("Overlap Triggered ! !"));

	if (ExtractionTimerHandle.IsValid()) return;

	ASpearmanCharacter* OverlappedCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (OverlappedCharacter)
	{
		CharacterToExtract = OverlappedCharacter;

		GetWorld()->GetTimerManager().SetTimer(ExtractionTimerHandle, this, &AExtractionPoint::Extraction, 5.f, false);
	}
}

void AExtractionPoint::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{ /* Server Only */

	ASpearmanCharacter* OverlappedCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (OverlappedCharacter)
	{
		CharacterToEscape = nullptr;
		ExtractionTimerHandle.Invalidate();
	}
}