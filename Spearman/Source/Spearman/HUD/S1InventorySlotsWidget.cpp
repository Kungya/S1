// Fill out your copyright notice in the Description page of Project Settings.

#include "S1InventorySlotsWidget.h"
#include "Components/UniformGridPanel.h"
#include "Spearman/HUD/S1InventorySlotWidget.h"
#include "Spearman/SpearComponents/InventoryComponent.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/Items/ItemInstance.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Spearman/HUD/S1InventoryItemInfoWidget.h"
#include "Spearman/HUD/S1DragDropOperation.h"

US1InventorySlotsWidget::US1InventorySlotsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void US1InventorySlotsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SlotWidgets.SetNum(Y_SIZE * X_SIZE);
	
	/* 5 * 10
	*  [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]
	*  [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]
	*  [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]
	*  [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]
	*  [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]
	*/

	APlayerController* PlayerController = GetOwningPlayer();

	for (int32 Y = 0; Y < Y_SIZE; Y++)
	{
		for (int32 X = 0; X < X_SIZE; X++)
		{
			int32 Idx = Y * X_SIZE + X;

			US1InventorySlotWidget* SlotWidget = CreateWidget<US1InventorySlotWidget>(PlayerController, SlotWidgetClass);

			SlotWidgets[Idx] = SlotWidget;
			Slots_GridPanel->AddChildToUniformGrid(SlotWidget, Y, X);
		}
	}

	ItemInfoWidgets.SetNum(Y_SIZE * X_SIZE);

	ASpearmanCharacter* OwnerSpearmanCharacter = Cast<ASpearmanCharacter>(PlayerController->GetCharacter());
	UInventoryComponent* Inventory = OwnerSpearmanCharacter->GetInventory();
	
	const TArray<UItemInstance*>& Items = Inventory->GetItems();
	// TODO : UItemInstance ���ο��� �κ��丮 ��� ��ġ�� �������� ��ġ���� �ε����� ��� �ִ°� �´�
	for (int32 i = 0; i < Items.Num(); i++)
	{
		UItemInstance* Item = Items[i];
		// i : 10�� ��(11��°, 2°�� ù��°�� ������), FIntPoint(0, 1) -> (x, y)
		FIntPoint ItemSlotPos = FIntPoint(i % X_SIZE, i / X_SIZE);
		OnInventoryItemInfoChanged(ItemSlotPos, Item);
	}
}

bool US1InventorySlotsWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);

	US1DragDropOperation* DragDrop = Cast<US1DragDropOperation>(InOperation);
	check(DragDrop);

	FVector2D MouseWidgetPos = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
	FVector2D ToWidgetPos = MouseWidgetPos - DragDrop->DeltaWidgetPos;
	FIntPoint ToSlotPos = FIntPoint(ToWidgetPos.X / InventorySlotSize.X, ToWidgetPos.Y / InventorySlotSize.Y);

	if (PrevDragOverSlotPos == ToSlotPos)
		return true;

	PrevDragOverSlotPos = ToSlotPos;

	// TODO : 

	return false;
}

void US1InventorySlotsWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	FinishDrag();
}

bool US1InventorySlotsWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	
	US1DragDropOperation* DragDrop = Cast<US1DragDropOperation>(InOperation);
	
	if (DragDrop)
	{
		FVector2D MouseWidgetPos = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
		FVector2D ToWidgetPos = MouseWidgetPos - DragDrop->DeltaWidgetPos;
		FIntPoint ToItemSlotPos = FIntPoint(ToWidgetPos.X / InventorySlotSize.X, ToWidgetPos.Y / InventorySlotSize.Y);

		if (DragDrop->FromItemSlotPos != ToItemSlotPos)
		{
			OnInventoryItemInfoChanged(DragDrop->FromItemSlotPos, nullptr);
			OnInventoryItemInfoChanged(ToItemSlotPos, DragDrop->ItemInstance);
		}
	}
	
	return false;
}

void US1InventorySlotsWidget::OnInventoryItemInfoChanged(const FIntPoint& InItemSlotPos, UItemInstance* Item)
{
	// 2d idx -> 1d idx
	int32 SlotIdx = InItemSlotPos.Y * X_SIZE + InItemSlotPos.X;

	if (US1InventoryItemInfoWidget* ItemInfoWidget = ItemInfoWidgets[SlotIdx])
	{
		if (Item == nullptr)
		{
			ItemInfos_CanvasPanel->RemoveChild(ItemInfoWidget);
			ItemInfoWidgets[SlotIdx] = nullptr;
		}
	}
	else
	{
		ItemInfoWidget = CreateWidget<US1InventoryItemInfoWidget>(GetOwningPlayer(), ItemInfoWidgetClass);
		ItemInfoWidgets[SlotIdx] = ItemInfoWidget;

		UCanvasPanelSlot* CanvasPanelSlot = ItemInfos_CanvasPanel->AddChildToCanvas(ItemInfoWidget);
		CanvasPanelSlot->SetAutoSize(true);
		// Slot �� ĭ�� ũ��� 50x50
		CanvasPanelSlot->SetPosition(FVector2D(InItemSlotPos.X * 50, InItemSlotPos.Y * 50));

		// Caching
		// TODO : ItemCount ��å
		ItemInfoWidget->Init(this, Item, 1);
	}
}

void US1InventorySlotsWidget::FinishDrag()
{
	PrevDragOverSlotPos = FIntPoint(-1, -1);
}