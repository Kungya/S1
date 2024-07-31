// Fill out your copyright notice in the Description page of Project Settings.

#include "S1InventorySlotsWidget.h"
#include "Components/UniformGridPanel.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Spearman/SpearComponents/InventoryComponent.h"
#include "Spearman/Items/ItemInstance.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Spearman/HUD/S1InventorySlotWidget.h"
#include "Spearman/HUD/S1InventoryItemInfoWidget.h"
#include "Spearman/HUD/S1DragDropOperation.h"
#include "Components/UniformGridSlot.h"

US1InventorySlotsWidget::US1InventorySlotsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void US1InventorySlotsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SpearmanPlayerController = (SpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(GetOwningPlayer()) : SpearmanPlayerController;
	Inventory = (Inventory == nullptr) ? SpearmanPlayerController->GetInventory() : Inventory;

	InitItemSlotWidget();
	
	ItemInfoWidgets.SetNum(Y_SIZE * X_SIZE);

	/* If default items exist when begin play */
	const TArray<UItemInstance*>& InventoryArray = Inventory->GetInventoryArray();
	for (int32 i = 0; i < InventoryArray.Num(); i++)
	{  // TODO : UItemInstance 내부에서 인벤토리 어느 위치에 아이템을 배치할지 인덱스를 들고 있는게 나을수도 있다
		UItemInstance* ItemInstance = InventoryArray[i];

		if (ItemInstance)
		{
			FIntPoint ItemSlotPos = FIntPoint(i % X_SIZE, i / X_SIZE);
			OnInventoryItemInfoChanged(ItemSlotPos, ItemInstance);
		}
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
			const int32 ToItemSlotIdx = ToItemSlotPos.Y * X_SIZE + ToItemSlotPos.X;
			if (ToItemSlotIdx < ItemInfoWidgets.Num())
			{
				US1InventoryItemInfoWidget* ToItemInfoWidget = ItemInfoWidgets[ToItemSlotIdx];

				UItemInstance* ToItemInstance = nullptr;
				if (ToItemInfoWidget)
				{ // Swap if ToItemSlot is valid
					ToItemInstance = ToItemInfoWidget->GetItemInstance();
				}
				/* Swap */
				OnInventoryItemInfoChanged(DragDrop->FromItemSlotPos, ToItemInstance);
				OnInventoryItemInfoChanged(ToItemSlotPos, DragDrop->ItemInstance);
			}
		}
	}
	
	return false;
}

void US1InventorySlotsWidget::InitItemSlotWidget()
{
	SlotWidgets.SetNum(X_SIZE * Y_SIZE);

	for (int32 Y = 0; Y < Y_SIZE; Y++)
	{
		for (int32 X = 0; X < X_SIZE; X++)
		{
			int32 Idx = Y * X_SIZE + X;

			US1InventorySlotWidget* SlotWidget = CreateWidget<US1InventorySlotWidget>(SpearmanPlayerController, SlotWidgetClass);
			SlotWidgets[Idx] = SlotWidget;
			Slots_GridPanel->AddChildToUniformGrid(SlotWidget, Y, X);
		}
	}
}

void US1InventorySlotsWidget::OnInventoryItemInfoChanged(const FIntPoint& InItemSlotPos, UItemInstance* Item)
{ // InItemSlotPos 위치에 있는 ItemInstance를 Item으로 바꾸겠다
	// 2d idx -> 1d idx
	const int32 SlotIdx = InItemSlotPos.Y * X_SIZE + InItemSlotPos.X;

	US1InventoryItemInfoWidget* ItemInfoWidget = ItemInfoWidgets[SlotIdx];
	if (ItemInfoWidget)
	{
		if (Item == nullptr)
		{ // To nullptr
			ItemInfos_CanvasPanel->RemoveChild(ItemInfoWidget);
			ItemInfoWidgets[SlotIdx] = nullptr;
		}
		else
		{ // To ItemInstance
			ItemInfoWidget->Init(this, Item, 1);
		}
	}
	else
	{ // Init new ItemInfo Widget in NativeConstruct()
		ItemInfoWidget = CreateWidget<US1InventoryItemInfoWidget>(SpearmanPlayerController, ItemInfoWidgetClass);
		ItemInfoWidgets[SlotIdx] = ItemInfoWidget;

		UCanvasPanelSlot* CanvasPanelSlot = ItemInfos_CanvasPanel->AddChildToCanvas(ItemInfoWidget);
		CanvasPanelSlot->SetAutoSize(true);
		CanvasPanelSlot->SetPosition(FVector2D(InItemSlotPos.X * 50, InItemSlotPos.Y * 50));

		ItemInfoWidget->Init(this, Item, 1);
	}
}

void US1InventorySlotsWidget::UpdateItemInfoWidget(const int32 InventoryArrayIndex)
{
	const TArray<UItemInstance*>& InventoryArray = Inventory->GetInventoryArray();
	UItemInstance* ItemInstance = InventoryArray[InventoryArrayIndex];
	
	int32 ItemInfoWidgetEmptyIndex = -1;
	for (int32 idx = 0; idx < ItemInfoWidgets.Num(); idx++)
	{
		if (ItemInfoWidgets[idx] == nullptr)
		{
			ItemInfoWidgetEmptyIndex = idx;
			break;
		}
	}
	// i : 10일 때(11번째, 2째줄 첫번째로 가야함), FIntPoint(0, 1) -> (x, y)
	FIntPoint ItemSlotPos = FIntPoint(ItemInfoWidgetEmptyIndex % X_SIZE, ItemInfoWidgetEmptyIndex / X_SIZE);
	OnInventoryItemInfoChanged(ItemSlotPos, ItemInstance);
}

void US1InventorySlotsWidget::FinishDrag()
{
	PrevDragOverSlotPos = FIntPoint(-1, -1);
}

void US1InventorySlotsWidget::GetLogfromSlotsWidget()
{
	UE_LOG(LogTemp, Warning, TEXT("Can Access !!"));
}