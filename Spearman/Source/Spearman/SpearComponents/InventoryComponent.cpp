// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"
#include "Spearman/Items/ItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Spearman/HUD/SpearmanHUD.h"
#include "Spearman/HUD/CharacterOverlay.h"
#include "Spearman/HUD/S1InventoryWidget.h"
#include "Spearman/HUD/S1InventorySlotsWidget.h"
#include "Spearman/Items/Item.h"
#include "Spearman/GameInstance/S1GameInstance.h"
#include "Spearman/PlayerState/SpearmanPlayerState.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerSpearmanPlayerController = Cast<ASpearmanPlayerController>(GetOwner());
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UInventoryComponent, InventoryArray, COND_OwnerOnly);
}

bool UInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	bWroteSomething |= Channel->ReplicateSubobjectList(InventoryArray, *Bunch, *RepFlags);

	return bWroteSomething;
}

void UInventoryComponent::AddItem(UItemInstance* InItemInstance)
{ /* Server Only */
	if (InventoryArray.Num() >= 50) return;
	
	UE_LOG(LogTemp, Warning, TEXT("AddItem"));

	// Change Outer from Item to InventoryComponent for Object Replication (Lifecycle)
	InItemInstance->Rename(nullptr, this);

	if (CachedInvalidIndex.IsEmpty())
	{
		InItemInstance->InventoryIdx = InventoryArray.Add(InItemInstance);
	}
	else
	{
		const int32 InvalidIndex = CachedInvalidIndex.Top();
		CachedInvalidIndex.Pop();

		InItemInstance->InventoryIdx = InvalidIndex;
		InventoryArray[InvalidIndex] = InItemInstance;
	}

	if (SpearmanCharacter && SpearmanCharacter->IsLocallyControlled())
	{
		US1InventorySlotsWidget* SlotsWidget = OwnerSpearmanPlayerController->GetSpearmanHUD()->CharacterOverlay->InventoryWidget->InventorySlotsWidget;
		if (SlotsWidget)
		{
			SlotsWidget->UpdateItemInfoWidget(InItemInstance->InventoryIdx);
		}
	}
}

void UInventoryComponent::RemoveItem(const int32 IdxToRemove)
{
	InventoryArray[IdxToRemove] = nullptr;
	CachedInvalidIndex.Push(IdxToRemove);
}

void UInventoryComponent::EmptyInventory()
{
	for (int32 idx = 0; InventoryArray.Num(); idx++)
	{
		if (InventoryArray[idx])
		{
			RemoveItem(idx);
		}
	}
}

void UInventoryComponent::ServerDropItem_Implementation(const int32 IdxToDrop)
{ /* Server Only */
	UItemInstance* ItemInstance = InventoryArray[IdxToDrop];

	RemoveItem(IdxToDrop);
	
	S1GameInstance = (S1GameInstance == nullptr) ? Cast<US1GameInstance>(GetWorld()->GetGameInstance()) : S1GameInstance;

	AItem* ItemToSpawn = GetWorld()->SpawnActorDeferred<AItem>(ItemClass, SpearmanCharacter->GetActorTransform());
	if (ItemToSpawn)
	{
		ItemToSpawn->Init(ItemInstance);
		ItemInstance->Rename(nullptr, ItemToSpawn);
		const TArray<UStaticMesh*>& StaticMeshAssets = S1GameInstance->GetStaticMeshAssets();
		ItemToSpawn->GetItemMesh()->SetStaticMesh(StaticMeshAssets[ItemInstance->ItemDataIdx]);
		ItemToSpawn->FinishSpawning(SpearmanCharacter->GetActorTransform());
	}
}

void UInventoryComponent::ServerSellItem_Implementation(const int32 IdxToSell)
{ /* Server Only */
	// TODO : Item 삭제후, 가격만큼 Balance 증가
	UItemInstance* ItemInstance = InventoryArray[IdxToSell];

	RemoveItem(IdxToSell);

	SpearmanPlayerState = OwnerSpearmanPlayerController->GetPlayerState<ASpearmanPlayerState>();
	if (SpearmanPlayerState)
	{
		SpearmanPlayerState->SetBalance(SpearmanPlayerState->GetBalance() + ItemInstance->Cost);
	}
}

void UInventoryComponent::OnRep_InventoryArray(TArray<UItemInstance*> LastInventoryArray)
{
	int32 LastInventoryArrayCount = 0;
	for (int32 idx = 0; idx < LastInventoryArray.Num(); idx++)
	{
		if (LastInventoryArray[idx] != nullptr)
			LastInventoryArrayCount++;
	}

	int32 InventoryArrayCount = 0;
	for (int32 idx = 0; idx < InventoryArray.Num(); idx++)
	{
		if (InventoryArray[idx] != nullptr)
			InventoryArrayCount++;
	}

	if (LastInventoryArrayCount > InventoryArrayCount)
	{ // if Drop, it's Already Updated before ServerRPC 
		return;
	}

	int32 ReplicatedElementIndex = InventoryArray.Num() - 1;
	for (int32 idx = 0; idx < LastInventoryArray.Num(); idx++)
	{
		if (LastInventoryArray[idx] == nullptr && InventoryArray[idx] != nullptr)
		{ // find Replicated element !
			ReplicatedElementIndex = idx;
			break;
		}
	}

	if (OwnerSpearmanPlayerController)
	{
		US1InventorySlotsWidget* SlotsWidget = OwnerSpearmanPlayerController->GetSpearmanHUD()->CharacterOverlay->InventoryWidget->InventorySlotsWidget;
		if (SlotsWidget)
		{
			SlotsWidget->UpdateItemInfoWidget(ReplicatedElementIndex);
		}
	}
}