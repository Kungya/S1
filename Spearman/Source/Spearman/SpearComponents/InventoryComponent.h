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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AddItem(UItemInstance* InIntemInstance);

	void UpdateHUDInventory();
private:
	// caching owner
	ASpearmanCharacter* Character;
	
	UPROPERTY(ReplicatedUsing = OnRep_InventoryArray)
	TArray<UItemInstance*> InventoryArray;

	UFUNCTION()
	void OnRep_InventoryArray();
	
	// ***********
	// TODO : ItemInstance�� �κ��丮 �ȿ� ���� �� Outer�� ����������ϰ�, ���� ���� Outer�� ����������Ѵ�

public:

	FORCEINLINE const TArray<UItemInstance*>& GetInventoryArray() { return InventoryArray; }

};