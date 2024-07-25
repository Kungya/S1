// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "S1UserWidget.h"
#include "ItemDropCanvasWidget.generated.h"

class US1DragDropOperation;
class ASpearmanCharacter;
class UInventoryComponent;

UCLASS()
class SPEARMAN_API UItemDropCanvasWidget : public US1UserWidget
{
	GENERATED_BODY()
	
public:

protected:
	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	UPROPERTY()
	ASpearmanCharacter* SpearmanCharacter;

	UPROPERTY()
	UInventoryComponent* Inventory;

	/* Inventory size, Y * X */
	const int32 Y_SIZE = 5;
	const int32 X_SIZE = 10;
};
