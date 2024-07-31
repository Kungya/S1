// Fill out your copyright notice in the Description page of Project Settings.


#include "ExtractionPoint.h"
#include "Components/SphereComponent.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Spearman/GameInstance/ExtractionResultSubsystem.h"
#include "Spearman/SpearComponents/InventoryComponent.h"
#include "Spearman/Items/ItemInstance.h"


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
	}
}

void AExtractionPoint::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{ /* Server Only */
	ASpearmanCharacter* OverlappedCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (OverlappedCharacter)
	{
		CharacterToExtract = nullptr;
		ExtractionTimerHandle.Invalidate();
	}
}

void AExtractionPoint::Extraction()
{ /* Server Only */
	if (CharacterToExtract == nullptr) return;

	ASpearmanPlayerController* SpearmanPlayerController = Cast<ASpearmanPlayerController>(CharacterToExtract->Controller);
	if (SpearmanPlayerController)
	{
		APlayerState* PlayerState = SpearmanPlayerController->GetPlayerState<APlayerState>();
		if (PlayerState)
		{
			UExtractionResultSubsystem* ExtractionResultSubsystem = GetGameInstance()->GetSubsystem<UExtractionResultSubsystem>();
			if (ExtractionResultSubsystem)
			{ /* Warning : if TMap's 'key' is Already existed, 'value' will be replaced */
				/*UE_LOG(LogTemp, Warning, TEXT("before Inventory size : %d"), SpearmanPlayerController->GetInventory()->GetInventoryArray().Num());
				ExtractionResultSubsystem->SavedInventories.Add(PlayerState->GetPlayerId(), ::MoveTemp(SpearmanPlayerController->GetInventory()->GetInventoryArray()));
				UE_LOG(LogTemp, Warning, TEXT("Add ! now TMap, Inventory size : %d, %d"), ExtractionResultSubsystem->SavedInventories[PlayerState->GetPlayerId()].Num(), SpearmanPlayerController->GetInventory()->GetInventoryArray().Num());*/

				CharacterToExtract->Extract();
			}
		}
		
		// TODO : Save Inventroy TArray with Unique Id
	}
}