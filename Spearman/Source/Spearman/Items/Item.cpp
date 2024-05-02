// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"
#include "ItemInstance.h"

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

	ItemInstance = NewObject<UItemInstance>();
	ItemInstance->Init(100);
}

void AItem::Interact()
{ // must be called in server only (server RPC in spearmancharacter)

	Destroy();
}

