// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spearman/Interfaces/InteractableInterface.h"
#include "Item.generated.h"


class UTexture2D;
class UItemInstance;

UCLASS()
class SPEARMAN_API AItem : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
	
public:	
	AItem();

	virtual void Interact() override;
	// set num 1~5 for ItemInstance
	void Init(int32 num);
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	UPROPERTY(EditDefaultsOnly, Category = "Item")
	UStaticMeshComponent* ItemMesh;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Item")
	UItemInstance* ItemInstance = nullptr;

private:


public:	
	FORCEINLINE UItemInstance* GetItemInstance() const { return ItemInstance; }
};
