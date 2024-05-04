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

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
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

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UInventoryComponent::AddItem(UItemInstance* InItemInstance)
{ // must be called in server only
	if (InventoryArray.Num() >= 50) return;
	
	InItemInstance->Rename(nullptr, GetOwner());
	InventoryArray.Add(InItemInstance);

	UE_LOG(LogTemp, Warning, TEXT("Inventory Size : %d"), InventoryArray.Num());
}

void UInventoryComponent::UpdateHUDInventory()
{
	Character = (Character == nullptr) ? Cast<ASpearmanCharacter>(GetOwner()) : Character;
	if (Character)
	{
		US1InventorySlotsWidget* SlotsWidget = Character->SpearmanPlayerController->GetSpearmanHUD()->CharacterOverlay->InventoryWidget->InventorySlotsWidget;
		// SlotsWidget->
		// TODO : 
	}
}

void UInventoryComponent::OnRep_InventoryArray()
{
	UE_LOG(LogTemp, Warning, TEXT("Replicated !!"));

	UpdateHUDInventory();
	
}