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

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	SpearmanCharacter = (SpearmanCharacter == nullptr) ? Cast<ASpearmanCharacter>(GetOwner()) : SpearmanCharacter;
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
	
	// Change Outer from Item to Inventory for Object Replication
	InItemInstance->Rename(nullptr, GetOwner());

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
		UpdateHUDInventory();
	}
}

void UInventoryComponent::RemoveItem(const int32 IdxToRemove)
{
	InventoryArray[IdxToRemove] = nullptr;
	CachedInvalidIndex.Push(IdxToRemove);
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
		const TArray<UStaticMesh*>& StaticMeshAssets = S1GameInstance->GetStaticMeshAssets();
		ItemToSpawn->GetItemMesh()->SetStaticMesh(StaticMeshAssets[ItemInstance->ItemDataIdx]);
		ItemToSpawn->FinishSpawning(SpearmanCharacter->GetActorTransform());
	}
}

void UInventoryComponent::UpdateHUDInventory()
{ 
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
	{ // if Dropp, it's Already Updated, 
		return;
	}

	int32 ReplicatedIndex = InventoryArray.Num() - 1;
	for (int32 idx = 0; idx < LastInventoryArray.Num(); idx++)
	{
		if (LastInventoryArray[idx] == nullptr && InventoryArray[idx] != nullptr)
		{ // find Replicated element !
			ReplicatedIndex = idx;
			break;
		}
	}

	SpearmanCharacter = (SpearmanCharacter == nullptr) ? Cast<ASpearmanCharacter>(GetOwner()) : SpearmanCharacter;
	if (SpearmanCharacter)
	{
		US1InventorySlotsWidget* SlotsWidget = SpearmanCharacter->SpearmanPlayerController->GetSpearmanHUD()->CharacterOverlay->InventoryWidget->InventorySlotsWidget;
		if (SlotsWidget)
		{
			SlotsWidget->UpdateItemInfoWidget(ReplicatedIndex);
		}
	}
}