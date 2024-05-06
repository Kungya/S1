// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NetworkObject.generated.h"

/**
 * 
 */
UCLASS()
class SPEARMAN_API UNetworkObject : public UObject
{
	GENERATED_BODY()
	
public:	
	virtual UWorld* GetWorld() const;

	UFUNCTION(BlueprintPure, Category = "Network Object")
	AActor* GetOwningActor() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Network Object")
	void Destroy();

protected:
	virtual int32 GetFunctionCallspace(UFunction * Function, FFrame * Stack);
	virtual bool CallRemoteFunction(UFunction * Function, void* Parms, struct FOutParmRec* OutParms, FFrame * Stack);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	virtual void OnDestroyed();

public:
	FORCEINLINE virtual bool IsSupportedForNetworking() const { return true; }
};
