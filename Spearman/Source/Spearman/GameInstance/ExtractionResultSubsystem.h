// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ExtractionResultSubsystem.generated.h"

/* TODO : Persistent Data (After Extraction) */
class UItemInstance;

UCLASS()
class SPEARMAN_API UExtractionResultSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	TMap<int32, TArray<UItemInstance*>> SavedInventories;

protected:


private:



public:


};
