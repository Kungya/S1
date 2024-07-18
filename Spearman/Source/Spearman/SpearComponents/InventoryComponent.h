// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItemInstance;
class ASpearmanCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPEARMAN_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
public:	

	void AddItem(UItemInstance* InIntemInstance);

	void UpdateHUDInventory();

private:
	ASpearmanCharacter* SpearmanCharacter;
	
	UPROPERTY(ReplicatedUsing = OnRep_InventoryArray)
	TArray<UItemInstance*> InventoryArray;

	UFUNCTION()
	void OnRep_InventoryArray();
	
	/* You have to set Outer When you Add or Remove ItemInstance in Inventory,  */

public:

	FORCEINLINE const TArray<UItemInstance*>& GetInventoryArray() { return InventoryArray; }

};