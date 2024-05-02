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
	
	// Allows the Object to get a valid UWorld from it's outer.
	virtual UWorld* GetWorld() const override;

	UFUNCTION(BlueprintPure, Category = "My Object")
	AActor* GetOwningActor() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FORCEINLINE virtual bool IsSupportedForNetworking() const override;

	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;

	// Call "Remote" (aka, RPC) functions through the actors NetDriver
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack) override;

	/*
	* Optional
	* Since this is a replicated object, typically only the Server should create and destroy these
	* Provide a custom destroy function to ensure these conditions are met.
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "My Object")
	void Destroy();

protected:
	virtual void OnDestroyed();
};
