// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Spearman/Network/NetworkObject.h"
//#include "UObject/NoExportTypes.h"
#include "ItemInstance.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class SPEARMAN_API UItemInstance : public UNetworkObject
{
	GENERATED_BODY()
	
public:
	UItemInstance();

	void Init(int32 InItemId);
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;
	
public:
	UPROPERTY(Replicated, VisibleAnywhere)
	int32 ItemId = 0;

	UPROPERTY(Replicated, VisibleAnywhere)
	int32 ItemCost = 0;
	
};