// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemDropCanvasWidget.h"
#include "Spearman/HUD/S1DragDropOperation.h"
#include "Spearman/HUD/S1InventorySlotsWidget.h"
#include "Spearman/HUD/S1InventoryItemInfoWidget.h"
#include "Spearman/Items/ItemInstance.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/SpearComponents/InventoryComponent.h"

void UItemDropCanvasWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SpearmanCharacter = (SpearmanCharacter == nullptr) ? Cast<ASpearmanCharacter>(GetOwningPlayer()->GetCharacter()) : SpearmanCharacter;
	Inventory = (Inventory == nullptr) ? SpearmanCharacter->GetInventory() : Inventory;

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
	}

	return false;
}
