// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Spearman/HUD/S1UserWidget.h"
#include "S1InventorySlotsWidget.generated.h"

class ASpearmanPlayerController;
class UItemInstance;
class UInventoryComponent;
class US1InventorySlotWidget;
class US1InventoryItemInfoWidget;
class UUniformGridPanel;
class UCanvasPanel;


UCLASS()
class SPEARMAN_API US1InventorySlotsWidget : public US1UserWidget
{
	GENERATED_BODY()
	
public:
	US1InventorySlotsWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void UpdateItemInfoWidget(const int32 InventoryArrayIndex);

	void OnInventoryItemInfoChanged(const FIntPoint& InItemSlotPos, UItemInstance* Item);

protected:
	virtual void NativeConstruct() override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	void InitItemSlotWidget();

private:
	void FinishDrag();

	/* Caching */

	UPROPERTY()
	ASpearmanPlayerController* SpearmanPlayerController;

	UPROPERTY()
	UInventoryComponent* Inventory;

public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<US1InventorySlotWidget> SlotWidgetClass;

	UPROPERTY()
	TArray<US1InventorySlotWidget*> SlotWidgets;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<US1InventoryItemInfoWidget> ItemInfoWidgetClass;

	UPROPERTY()
	TArray<US1InventoryItemInfoWidget*> ItemInfoWidgets;

	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* Slots_GridPanel;

	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* ItemInfos_CanvasPanel;

private:
	FIntPoint PrevDragOverSlotPos = FIntPoint(-1, -1);
	// Inventory size, Y_SIZE * X_SIZE
	const int32 Y_SIZE = 5;
	const int32 X_SIZE = 10;

	const FIntPoint InventorySlotSize = FIntPoint(X_SIZE * Y_SIZE, X_SIZE * Y_SIZE);
};
