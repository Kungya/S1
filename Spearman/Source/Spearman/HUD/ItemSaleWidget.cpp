// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemSaleWidget.h"
#include "Spearman/HUD/S1DragDropOperation.h"
#include "Spearman/HUD/S1InventorySlotsWidget.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Spearman/SpearComponents/InventoryComponent.h"
#include "Spearman/Items/ItemInstance.h"

void UItemSaleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SpearmanPlayerController = (SpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(GetOwningPlayer()) : SpearmanPlayerController;
	Inventory = (Inventory == nullptr) ? SpearmanPlayerController->GetInventory() : Inventory;

	SetVisibility(ESlateVisibility::Visible);
}

bool UItemSaleWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	US1DragDropOperation* DragDrop = Cast<US1DragDropOperation>(InOperation);
	if (DragDrop)
	{
		FIntPoint ItemSlotPosToSell = DragDrop->FromItemSlotPos;
		UItemInstance* ItemInstanceToSell = DragDrop->ItemInstance;
		US1InventorySlotsWidget* InventorySlotsWidget = DragDrop->InventorySlotsWidget;

		if (ItemInstanceToSell == nullptr || InventorySlotsWidget == nullptr)
			return false;

		InventorySlotsWidget->OnInventoryItemInfoChanged(ItemSlotPosToSell, nullptr);

		int32 InventoryIdxToSell = ItemInstanceToSell->InventoryIdx;

		Inventory->ServerSellItem(InventoryIdxToSell);

		return true;
	}

	return false;
}