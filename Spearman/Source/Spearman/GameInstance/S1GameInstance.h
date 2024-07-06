// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "S1GameInstance.generated.h"

class UTexture2D;
class UDataTable;

USTRUCT()
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemData")
	int32 Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemData")
	int32 Cost;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemData")
	int32 Weight;

	// Idx in inventory widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemData")
	int32 InventoryIdx;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemData")
	int32 ItemDataIdx;
};

/**
 * 
 */
UCLASS()
class SPEARMAN_API US1GameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	US1GameInstance();

	virtual void Init() override;

	FItemData* GetItemData(int32 Id);

private:

	UPROPERTY(EditAnywhere)
	UDataTable* ItemDataTable;

	/* Mapping [Random Number] -> [DataTable Idx] */
	UPROPERTY()
	TArray<int32> ItemIds;

	/* set in BP */
	UPROPERTY(EditAnywhere, Category = "Item Data")
	TArray<UTexture2D*> TextureAssets;
	
	/* set in BP */
	UPROPERTY(EditAnywhere, Category = "Item Data")
	TArray<UStaticMesh*> StaticMeshAssets;

public:
	FORCEINLINE const TArray<int32>& GetItemIds() const { return ItemIds; }
	FORCEINLINE const TArray<UTexture2D*>& GetTextureAssets() const { return TextureAssets; }
	FORCEINLINE const TArray<UStaticMesh*>& GetStaticMeshAssets() const { return StaticMeshAssets; }
};