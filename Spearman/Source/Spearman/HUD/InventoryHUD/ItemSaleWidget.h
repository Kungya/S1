// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Spearman/HUD/S1UserWidget.h"
#include "ItemSaleWidget.generated.h"

class US1DragDropOperation;
class US1InventorySlotsWidget;
class ASpearmanPlayerController;
class UInventoryComponent;

UCLASS()
class SPEARMAN_API UItemSaleWidget : public US1UserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	UPROPERTY()
	ASpearmanPlayerController* SpearmanPlayerController;

	UPROPERTY()
	UInventoryComponent* Inventory;


};
