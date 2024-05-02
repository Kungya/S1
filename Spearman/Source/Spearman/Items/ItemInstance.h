// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ItemInstance.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class SPEARMAN_API UItemInstance : public UObject
{
	GENERATED_BODY()
	
public:
	UItemInstance();

public:
	void Init(int32 InItemId);
	
public:
	UPROPERTY(VisibleAnywhere)
	int32 ItemId = 0;

	UPROPERTY(VisibleAnywhere)
	int32 ItemCost = 0;
	
};