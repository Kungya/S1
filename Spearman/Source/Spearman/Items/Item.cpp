// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"
#include "ItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

AItem::AItem()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	RootComponent = ItemMesh;


}

void AItem::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// TODO : NewObject<UItemInstance>(this, UClass* Class);
		ItemInstance = NewObject<UItemInstance>(this);
		ItemInstance->Init(100);
	}
}

void AItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItem, ItemInstance);
}

bool AItem::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	bWroteSomething |= Channel->ReplicateSubobject(ItemInstance, *Bunch, *RepFlags);

	return bWroteSomething;
}

void AItem::Interact()
{ // must be called in server only (server RPC in spearmancharacter)

	Destroy();
}

