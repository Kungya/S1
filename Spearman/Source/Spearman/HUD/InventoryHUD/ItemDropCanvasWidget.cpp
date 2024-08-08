// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemDropCanvasWidget.h"
#include "Spearman/HUD/InventoryHUD/S1DragDropOperation.h"
#include "Spearman/HUD/InventoryHUD/S1InventorySlotsWidget.h"
#include "Spearman/HUD/InventoryHUD/S1InventoryItemInfoWidget.h"
#include "Spearman/Items/ItemInstance.h"
#include "Spearman/SpearComponents/InventoryComponent.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"

void UItemDropCanvasWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SpearmanPlayerController = (SpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(GetOwningPlayer()) : SpearmanPlayerController;
	Inventory = (Inventory == nullptr) ? SpearmanPlayerController->GetInventory() : Inventory;

	SetVisibility(ESlateVisibility::Visible);
}

bool UItemDropCanvasWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	US1DragDropOperation* DragDrop = Cast<US1DragDropOperation>(InOperation);
	if (DragDrop)
	{
		FIntPoint ItemSlotPosToDrop = DragDrop->FromItemSlotPos;
		UItemInstance* ItemInstanceToDrop = DragDrop->ItemInstance;
		US1InventorySlotsWidget* InventorySlotsWidget = DragDrop->InventorySlotsWidget;

		if (ItemInstanceToDrop == nullptr || InventorySlotsWidget == nullptr)
			return false;		                               

		InventorySlotsWidget->OnInventoryItemInfoChanged(ItemSlotPosToDrop, nullptr);
		
		int32 InventoryIdxToDrop = ItemInstanceToDrop->InventoryIdx;

		Inventory->ServerDropItem(InventoryIdxToDrop);

		return true;
	}

	return false;
}
