// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spearman/Interfaces/InteractableInterface.h"
#include "Item.generated.h"


// Deprecated
USTRUCT(BlueprintType)
struct FItemData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSubclassOf<class AItem> ItemClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UTexture2D* ItemImage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float ItemCost;
};

UCLASS()
class SPEARMAN_API AItem : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
	
public:	
	AItem();

	virtual void Interact() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Item")
	UStaticMeshComponent* ItemMesh;

	/*UPROPERTY(EditDefaultsOnly, Category = "Item")
	FItemData ItemData;*/

	UPROPERTY()
	class UItemInstance* ItemInstance;

private:


public:	

	FORCEINLINE UItemInstance* GetItemInstance() const { return ItemInstance; }

};
