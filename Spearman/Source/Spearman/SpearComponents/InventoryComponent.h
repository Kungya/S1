// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItemInstance;
class ASpearmanCharacter;
class AItem;
class US1GameInstance;

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

	void RemoveItem(const int32 IdxToRemove);

	UFUNCTION(Server, Reliable)
	void ServerDropItem(const int32 IdxToDrop);

	void UpdateHUDInventory();

private:
	ASpearmanCharacter* SpearmanCharacter;
	
	UPROPERTY(ReplicatedUsing = OnRep_InventoryArray)
	TArray<UItemInstance*> InventoryArray;

	UPROPERTY()
	TArray<int32> CachedInvalidIndex;
	
	UFUNCTION()
	void OnRep_InventoryArray(TArray<UItemInstance*> LastInventoryArray);
	
	UPROPERTY(EditAnywhere, Category = "Drop")
	TSubclassOf<AItem> ItemClass;

	UPROPERTY()
	US1GameInstance* S1GameInstance;

	/* You have to set Outer When you Add or Remove ItemInstance in Inventory,  */

public:

	FORCEINLINE TArray<UItemInstance*>& GetInventoryArray() { return InventoryArray; }

};