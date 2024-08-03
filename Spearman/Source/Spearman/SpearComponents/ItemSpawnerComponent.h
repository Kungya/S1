// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemSpawnerComponent.generated.h"

class AItem;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPEARMAN_API UItemSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UItemSpawnerComponent();

	void SpawnItem();
	void SpawnItemTimer();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Drop")
	TSubclassOf<AItem> ItemClass;

public:	
	
};
