// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RewindableInterface.generated.h"

class UBoxComponent;
class UHistoryComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class URewindableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SPEARMAN_API IRewindableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	virtual TArray<UBoxComponent*>& GetHitBoxArray() = 0;
	virtual UHistoryComponent* GetHistory() = 0;

};
