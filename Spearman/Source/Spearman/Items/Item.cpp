// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"
#include "ItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

AItem::AItem()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	SetRootComponent(SceneComponent);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(GetRootComponent());
}

void AItem::BeginPlay()
{
	Super::BeginPlay();

	if (!ItemInstance && HasAuthority())
	{ // default ItemInstance for test
		ItemInstance = NewObject<UItemInstance>(this);
		ItemInstance->Init(3);
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

void AItem::Init(int32 num)
{ // Server Only
	if (ItemInstance == nullptr)
	{
		ItemInstance = NewObject<UItemInstance>(this);
		ItemInstance->Init(num);
	}
}

void AItem::Interact()
{ // Server Only

	Destroy();
}