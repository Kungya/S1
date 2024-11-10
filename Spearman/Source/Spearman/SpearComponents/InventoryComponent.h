// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItemInstance;
class ASpearmanCharacter;
class ASpearmanPlayerController;
class AItem;
class US1GameInstance;
class ASpearmanPlayerState;

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

	bool AddItem(UItemInstance* InIntemInstance);

	void RemoveItem(const int32 IdxToRemove);

	void EmptyInventory();

	UFUNCTION(Server, Reliable)
	void ServerDropItem(const int32 IdxToDrop);

	UFUNCTION(Server, Reliable)
	void ServerSellItem(const int32 IdxToSell);
	
private:
	UPROPERTY()
	ASpearmanCharacter* SpearmanCharacter;

	UPROPERTY()
	ASpearmanPlayerController* OwnerSpearmanPlayerController;

	UPROPERTY()
	ASpearmanPlayerState* SpearmanPlayerState;
	
	UPROPERTY(ReplicatedUsing = OnRep_InventoryArray)
	TArray<UItemInstance*> InventoryArray;

	UPROPERTY()
	TArray<int32> CachedInvalidIndex;

	// It isn't Num() of InventoryArray, Trace real count of ItemInstance except "nullptr" in InventoryArray
	int32 InventorySize = 0;
	
	UFUNCTION()
	void OnRep_InventoryArray(TArray<UItemInstance*> LastInventoryArray);
	
	UPROPERTY(EditAnywhere, Category = "Drop")
	TSubclassOf<AItem> ItemClass;

	UPROPERTY()
	US1GameInstance* S1GameInstance;

	/* You have to set Outer as Actor When you Add or Remove ItemInstance in Inventory, */

public:
	FORCEINLINE void SetSpearmanCharacter(ASpearmanCharacter* InSpearmanCharacter) { SpearmanCharacter = InSpearmanCharacter; }
	FORCEINLINE TArray<UItemInstance*>& GetInventoryArray() { return InventoryArray; }

};