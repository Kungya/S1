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
	ItemMesh->SetSimulatePhysics(true);
	ItemMesh->SetIsReplicated(true);
	
}

void AItem::BeginPlay()
{
	Super::BeginPlay();

	if (!ItemInstance && HasAuthority())
	{ /* Default ItemInstance for test */
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
{ /* Server Only */
	if (ItemInstance == nullptr)
	{
		ItemInstance = NewObject<UItemInstance>(this);
		ItemInstance->Init(num);
	}
}

void AItem::Init(UItemInstance* InItemInstance)
{ /* Server Only */
	if (ItemInstance == nullptr && InItemInstance)
	{
		ItemInstance = InItemInstance;
		ItemInstance->Rename(nullptr, this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Init in AItem FAULT"));
	}
}

void AItem::Interact()
{ /* Server Only */
	Destroy();
}