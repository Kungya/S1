// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Spearman/Network/NetworkObject.h"
//#include "UObject/NoExportTypes.h"
#include "ItemInstance.generated.h"


class UTexture2D;
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
	
public:
	UPROPERTY(Replicated)
	int32 Id;

	UPROPERTY(Replicated)
	int32 Cost;

	UPROPERTY(Replicated)
	int32 Weight;

	UPROPERTY(Replicated)
	UTexture2D* Icon;

	// TODO : Data Table 추가시 설정
	UPROPERTY(Replicated)
	int32 InventoryIdx = -1;

private:

};