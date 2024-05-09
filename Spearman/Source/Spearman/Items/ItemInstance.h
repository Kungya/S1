// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Spearman/Network/NetworkObject.h"
#include "ItemInstance.generated.h"

class AItem;
class UTexture2D;
class US1GameInstance;
/**
 * 
 */
UCLASS(BlueprintType)
class SPEARMAN_API UItemInstance : public UNetworkObject
{
	GENERATED_BODY()
	
public:
	UItemInstance();

	void Init(int32 num);
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;
	
	virtual void PostInitProperties() override;

	UFUNCTION()
	void OnRep_ItemDataIdx();

public:
	UPROPERTY(Replicated)
	int32 Id;

	UPROPERTY(Replicated)
	int32 Cost;

	UPROPERTY(Replicated)
	int32 Weight;

	// TODO : Data Table 추가시 설정
	UPROPERTY(Replicated)
	int32 InventoryIdx = -1;

	UPROPERTY(ReplicatedUsing = OnRep_ItemDataIdx)
	int32 ItemDataIdx = -1;

	UPROPERTY(Replicated)
	UTexture2D* Icon;

private:
	/* Outer */
	UPROPERTY()
	AItem* Item;

	UPROPERTY()
	US1GameInstance* S1GameInstance;

public:
	FORCEINLINE UTexture2D* GetIcon() const { return Icon; }


};