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

//void AItem::Multicast_SetStaticMesh_Implementaiton()
//{
//	ItemMesh->SetStaticMesh()
//}

void AItem::Init(int32 num)
{ // Server Only
	if (ItemInstance == nullptr)
	{
		ItemInstance = NewObject<UItemInstance>(this);
		ItemInstance->Init(num);
		//UStaticMesh* StaticMesh = ItemInstance->GetStaticMesh();
		//if (StaticMesh)
		//{ // 서버에서만 설정을해서 안보는거였음 -> 모든 클라와 서버에서 StaticMesh Array를 들고 있다가, iDX만 넘겨줘서 그걸로 설정하면 어떨까
		//	ItemMesh->SetStaticMesh(StaticMesh);
		//}
	}
}

void AItem::Interact()
{ // Server Only

	Destroy();
}