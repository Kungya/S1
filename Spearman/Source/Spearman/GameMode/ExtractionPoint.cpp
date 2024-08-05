// Fill out your copyright notice in the Description page of Project Settings.


#include "ExtractionPoint.h"
#include "Components/SphereComponent.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Spearman/SpearComponents/InventoryComponent.h"
#include "Spearman/Items/ItemInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Spearman/GameMode/SpearmanGameMode.h"
#include "Spearman/HUD/SpearmanHUD.h"
#include "Spearman/HUD/ExtractionNoticeWidget.h"


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

void AExtractionPoint::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{ /* Server Only */
	if (ExtractionTimerHandle.IsValid()) return;

	ASpearmanCharacter* OverlappedCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (OverlappedCharacter)
	{
		CharacterToExtract = OverlappedCharacter;

		GetWorld()->GetTimerManager().SetTimer(ExtractionTimerHandle, this, &AExtractionPoint::Extraction, 5.f, false);

		if (OverlappedCharacter->SpearmanPlayerController)
		{
			OverlappedCharacter->SpearmanPlayerController->ClientSetExtractionNotice();
		}
	}
}

void AExtractionPoint::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{ /* Server Only */
	if (CharacterToExtract.IsValid())
	{
		if (CharacterToExtract.Get() == OtherActor)
		{
			if (CharacterToExtract->SpearmanPlayerController)
			{
				CharacterToExtract->SpearmanPlayerController->ClientClearExtractionNotice();
			}

			CharacterToExtract = nullptr;

			if (GetWorldTimerManager().IsTimerActive(ExtractionTimerHandle))
			{
				GetWorldTimerManager().ClearTimer(ExtractionTimerHandle);
			}
		}
	}
}

void AExtractionPoint::Extraction()
{ /* Server Only */
	if (!CharacterToExtract.IsValid()) return;

	CharacterToExtract->Extract();

	ASpearmanPlayerController* SpearmanPlayerController = Cast<ASpearmanPlayerController>(CharacterToExtract->Controller);
	if (SpearmanPlayerController)
	{
		ASpearmanGameMode* SpearmanGameMode = Cast<ASpearmanGameMode>(UGameplayStatics::GetGameMode(this));
		if (SpearmanGameMode)
		{
			SpearmanGameMode->WinnerList.Add(SpearmanPlayerController);
		}
	}
}