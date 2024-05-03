// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemInstance.h"

UItemInstance::UItemInstance()
{

}

void UItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

void UItemInstance::Init(int32 InItemId)
{
	if (InItemId <= 0) return;

	ItemId = InItemId;
	ItemCost = 20'000;


	// TODO : many default setting...
}