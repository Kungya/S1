// Fill out your copyright notice in the Description page of Project Settings.

#include "S1InventorySlotsWidget.h"
#include "Components/UniformGridPanel.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/SpearComponents/InventoryComponent.h"
#include "Spearman/Items/ItemInstance.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Spearman/HUD/S1InventorySlotWidget.h"
#include "Spearman/HUD/S1InventoryItemInfoWidget.h"
#include "Spearman/HUD/S1DragDropOperation.h"
#include "Components/UniformGridSlot.h"
#include "Components/OverlaySlot.h"

US1InventorySlotsWidget::US1InventorySlotsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void US1InventorySlotsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	/*UOverlaySlot* OverlaySlot_Slots_GridPanel = Cast<UOverlaySlot>(Slots_GridPanel->Slot);
	OverlaySlot_Slots_GridPanel->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
	OverlaySlot_Slots_GridPanel->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
	
	UOverlaySlot* OverlaySlot_ItemInfos_CanvasPanel = Cast<UOverlaySlot>(ItemInfos_CanvasPanel->Slot);
	OverlaySlot_ItemInfos_CanvasPanel->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
	OverlaySlot_ItemInfos_CanvasPanel->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);*/

	SlotWidgets.SetNum(X_SIZE * Y_SIZE);

	PlayerController = (PlayerController == nullptr) ? GetOwningPlayer() : PlayerController;

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

	SpearmanCharacter = (SpearmanCharacter == nullptr) ? Cast<ASpearmanCharacter>(PlayerController->GetCharacter()) : SpearmanCharacter;
	Inventory = (Inventory == nullptr) ? SpearmanCharacter->GetInventory() : Inventory;
	
	const TArray<UItemInstance*>& InventoryArray = Inventory->GetInventoryArray();
	// TODO : UItemInstance 내부에서 인벤토리 어느 위치에 아이템을 배치할지 인덱스를 들고 있는게 나을수도 있다
	for (int32 i = 0; i < InventoryArray.Num(); i++)
	{
		UItemInstance* ItemInstance = InventoryArray[i];
		// i : 10일 때(11번째, 2째줄 첫번째로 가야함), FIntPoint(0, 1) -> (x, y)
		FIntPoint ItemSlotPos = FIntPoint(i % X_SIZE, i / X_SIZE);
		OnInventoryItemInfoChanged(ItemSlotPos, ItemInstance);
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
			// OnInventoryItemInfoChanged(A, B) -> A위치에 있는걸 ItemInstance B로 바꾸겠다
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

	US1InventoryItemInfoWidget* ItemInfoWidget = ItemInfoWidgets[SlotIdx];
	if (ItemInfoWidget)
	{
		if (Item == nullptr)
		{
			ItemInfos_CanvasPanel->RemoveChild(ItemInfoWidget);
			ItemInfoWidgets[SlotIdx] = nullptr;
		}
		else
		{
			// TODO : else 쪽에 있는거 긁어와서 Update
			// TODO : ItemCount 정책
			ItemInfoWidget->Init(this, Item, 1);
		}
	}
	else
	{
		ItemInfoWidget = CreateWidget<US1InventoryItemInfoWidget>(PlayerController, ItemInfoWidgetClass);
		ItemInfoWidgets[SlotIdx] = ItemInfoWidget;

		UCanvasPanelSlot* CanvasPanelSlot = ItemInfos_CanvasPanel->AddChildToCanvas(ItemInfoWidget);
		CanvasPanelSlot->SetAutoSize(true);
		// Slot 한 칸의 크기는 50x50
		CanvasPanelSlot->SetPosition(FVector2D(InItemSlotPos.X * 50, InItemSlotPos.Y * 50));

		// Caching
		// TODO : ItemCount 정책
		ItemInfoWidget->Init(this, Item, 1);
	}
}

void US1InventorySlotsWidget::UpdateItemInfoWidget()
{
	const TArray<UItemInstance*>& InventoryArray = Inventory->GetInventoryArray();
	for (int32 i = 0; i < InventoryArray.Num(); i++)
	{
		UItemInstance* ItemInstance = InventoryArray[i];
		// i : 10일 때(11번째, 2째줄 첫번째로 가야함), FIntPoint(0, 1) -> (x, y)
		FIntPoint ItemSlotPos = FIntPoint(i % X_SIZE, i / X_SIZE);
		OnInventoryItemInfoChanged(ItemSlotPos, ItemInstance);
	}
}

void US1InventorySlotsWidget::FinishDrag()
{
	PrevDragOverSlotPos = FIntPoint(-1, -1);
}

void US1InventorySlotsWidget::GetLogfromSlotsWidget()
{
	UE_LOG(LogTemp, Warning, TEXT("Can Access !!"));
}