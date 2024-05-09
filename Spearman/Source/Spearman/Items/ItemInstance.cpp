// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Spearman/GameInstance/S1GameInstance.h"
#include "Spearman/Items/Item.h"


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
	DOREPLIFETIME(UItemInstance, ItemDataIdx);
}

void UItemInstance::PostInitProperties()
{
	Super::PostInitProperties();

	Item = (Item == nullptr) ? GetTypedOuter<AItem>() : Item;
}

void UItemInstance::Init(int32 num)
{ // Server Only
	if (num <= 0) return;

	S1GameInstance = (S1GameInstance == nullptr) ? Cast<US1GameInstance>(GetWorld()->GetGameInstance()) : S1GameInstance;

	// Mapping [Random Number] to [Item Id]
	const TArray<int32>& ItemIds = S1GameInstance->GetItemIds();
	const int32 ItemId = ItemIds[num - 1];

	// TODO : consider Replication's unordered
	Id = S1GameInstance->GetItemData(ItemId)->Id;
	Cost = S1GameInstance->GetItemData(ItemId)->Cost;
	Weight = S1GameInstance->GetItemData(ItemId)->Weight;
	ItemDataIdx = S1GameInstance->GetItemData(ItemId)->ItemDataIdx;
	/*StaticMeshIdx = S1GameInstance->GetItemData(ItemId)->StaticMeshIdx;*/
	// Icon = S1GameInstance->GetItemData(ItemId)->Texture;
	// StaticMesh = S1GameInstance->GetItemData(ItemId)->StaticMesh;

	Item = (Item == nullptr) ? GetTypedOuter<AItem>() : Item;
	if (Item)
	{
		const TArray<UTexture2D*>& TextureAssets = S1GameInstance->GetTextureAssets();
		const TArray<UStaticMesh*>& StaticMeshAssets = S1GameInstance->GetStaticMeshAssets();
		Icon = TextureAssets[ItemDataIdx];
		Item->GetItemMesh()->SetStaticMesh(StaticMeshAssets[ItemDataIdx]);
	}
}

void UItemInstance::OnRep_ItemDataIdx()
{ // Client Only, TODO : Init()에선 OwnerOnly로 할 경우 Owner가 Item, 즉 서버라 Client들은 Replicated받지 못함.
	// AddItem때 InventroyComponent의 TArray가 복제되면서 값이 넘어올듯.
	Item = (Item == nullptr) ? GetTypedOuter<AItem>() : Item;
	if (Item)
	{
		S1GameInstance = (S1GameInstance == nullptr) ? Cast<US1GameInstance>(GetWorld()->GetGameInstance()) : S1GameInstance;
		if (S1GameInstance)
		{
			UE_LOG(LogTemp, Warning, TEXT("OnRep, ItemDataIdx : %d"), ItemDataIdx);
			const TArray<UTexture2D*>& TextureAssets = S1GameInstance->GetTextureAssets();
			const TArray<UStaticMesh*>& StaticMeshAssets = S1GameInstance->GetStaticMeshAssets();
			Icon = TextureAssets[ItemDataIdx];
			Item->GetItemMesh()->SetStaticMesh(StaticMeshAssets[ItemDataIdx]);
		}
	}
}