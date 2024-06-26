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

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UInventoryComponent::AddItem(UItemInstance* InItemInstance)
{ // Server Only
	if (InventoryArray.Num() >= 50) return;
	
	// change Outer in (Item -> Inventory) for Object Replication
	InItemInstance->Rename(nullptr, GetOwner());
	const int32 Idx = InventoryArray.Add(InItemInstance);
	InItemInstance->InventoryIdx = Idx;

	UE_LOG(LogTemp, Warning, TEXT("Inventory Size : %d"), InventoryArray.Num());

	if (SpearmanCharacter && SpearmanCharacter->IsLocallyControlled())
	{
		UpdateHUDInventory();
	}
}

void UInventoryComponent::UpdateHUDInventory()
{ // should be called in ROLE_Authority or ROLE_AutonomousProxy because of PlayerConroller
	SpearmanCharacter = (SpearmanCharacter == nullptr) ? Cast<ASpearmanCharacter>(GetOwner()) : SpearmanCharacter;
	if (SpearmanCharacter)
	{
		US1InventorySlotsWidget* SlotsWidget = SpearmanCharacter->SpearmanPlayerController->GetSpearmanHUD()->CharacterOverlay->InventoryWidget->InventorySlotsWidget;
		if (SlotsWidget)
		{
			SlotsWidget->UpdateItemInfoWidget();
			UE_LOG(LogTemp, Warning, TEXT("UpdateHUDInventory"));
		}
	}
}

void UInventoryComponent::OnRep_InventoryArray()
{
	UpdateHUDInventory();
}