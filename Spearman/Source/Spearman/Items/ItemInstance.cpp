// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Spearman/GameInstance/S1GameInstance.h"


UItemInstance::UItemInstance()
{

}

void UItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UItemInstance, Id, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UItemInstance, Cost, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UItemInstance, Weight, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UItemInstance, InventoryIdx, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UItemInstance, Icon, COND_OwnerOnly);
}

void UItemInstance::Init(int32 num)
{
	if (num <= 0) return;

	US1GameInstance* S1GameInstance = Cast<US1GameInstance>(GetWorld()->GetGameInstance());

	switch (num)
	{
	case 1:
		Id = S1GameInstance->GetItemData(101)->Id;
		Cost = S1GameInstance->GetItemData(101)->Cost;
		Weight = S1GameInstance->GetItemData(101)->Weight;
		Icon = S1GameInstance->GetItemData(101)->Texture;
		break;
	case 2:
		Id = S1GameInstance->GetItemData(102)->Id;
		Cost = S1GameInstance->GetItemData(102)->Cost;
		Weight = S1GameInstance->GetItemData(102)->Weight;
		Icon = S1GameInstance->GetItemData(102)->Texture;
		break;
	case 3:
		Id = S1GameInstance->GetItemData(201)->Id;
		Cost = S1GameInstance->GetItemData(201)->Cost;
		Weight = S1GameInstance->GetItemData(201)->Weight;
		Icon = S1GameInstance->GetItemData(201)->Texture;
		break;
	case 4:
		Id = S1GameInstance->GetItemData(202)->Id;
		Cost = S1GameInstance->GetItemData(202)->Cost;
		Weight = S1GameInstance->GetItemData(202)->Weight;
		Icon = S1GameInstance->GetItemData(202)->Texture;
		break;
	case 5:
		Id = S1GameInstance->GetItemData(203)->Id;
		Cost = S1GameInstance->GetItemData(203)->Cost;
		Weight = S1GameInstance->GetItemData(203)->Weight;
		Icon = S1GameInstance->GetItemData(203)->Texture;
		break;
	}
}