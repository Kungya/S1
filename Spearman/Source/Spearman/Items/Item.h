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

	void Init(int32 num);
	virtual void Interact() override;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

private:

	UPROPERTY(VisibleAnywhere)
	USceneComponent* SceneComponent;

	UPROPERTY(VisibleAnywhere, Category = "Item")
	UStaticMeshComponent* ItemMesh;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Item")
	UItemInstance* ItemInstance;

public:	
	FORCEINLINE UItemInstance* GetItemInstance() const { return ItemInstance; }
	FORCEINLINE UStaticMeshComponent* GetItemMesh() const { return ItemMesh; }
};
