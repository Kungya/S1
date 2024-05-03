// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"
#include "Spearman/Items/ItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Spearman/Character/SpearmanCharacter.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Replicated is call in SpearmanCharacter, | SetIsReplicatedByDefault(true);
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

void UInventoryComponent::OnRep_InventoryArray()
{
	UE_LOG(LogTemp, Warning, TEXT("Replicated !!"));

	Character = (Character == nullptr) ? Cast<ASpearmanCharacter>(GetOwner()) : Character;
	if (Character)
	{
		//Character->
	}
}

