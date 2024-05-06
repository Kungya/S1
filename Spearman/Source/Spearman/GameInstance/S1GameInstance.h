// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "S1GameInstance.generated.h"

class UTexture2D;

USTRUCT()
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Cost;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Weight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 InventoryIdx;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Texture;


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
	class UDataTable* ItemDataTable;
};
