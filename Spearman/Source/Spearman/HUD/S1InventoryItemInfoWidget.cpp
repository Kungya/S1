// Fill out your copyright notice in the Description page of Project Settings.


#include "S1InventoryItemInfoWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Spearman/HUD/S1InventorySlotsWidget.h"
#include "Spearman/Items/ItemInstance.h"
#include "Spearman/HUD/S1DragDropOperation.h"
#include "Spearman/HUD/S1ItemDragWidget.h"

US1InventoryItemInfoWidget::US1InventoryItemInfoWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void US1InventoryItemInfoWidget::Init(US1InventorySlotsWidget* InSlotsWidget, UItemInstance* InItemInstance, int32 InItemCount)
{ // caching
	SlotsWidget = InSlotsWidget;
	ItemInstance = InItemInstance;
	ItemCount = InItemCount;
	Icon_Image->SetBrushFromTexture(InItemInstance->Icon, true);

	// TODO : ItemInstance, Count�� �ٲ�� -> Icon_Image, Hover_Image, Count_Text�� �ٲ�����Ѵ�
	// Icon_Image, Hover_Image�� ItemInstance
}

void US1InventoryItemInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Count_Text->SetText(FText::GetEmpty());
}

void US1InventoryItemInfoWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	Hover_Image->SetRenderOpacity(1.f);

	// TODO : ���콺�� ������ �߰����� UI
}

void US1InventoryItemInfoWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	Hover_Image->SetRenderOpacity(0.f);
}

FReply US1InventoryItemInfoWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{ // DragOperation�� �ްڴ�
		Reply.DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	const FIntPoint InventorySlotSize = FIntPoint(50, 50);

	// Slots���� ��� Slot�� Ŭ���ߴ��� ��ǥ ��ȯ�ؼ� ������
	FVector2D MouseWidgetPos = SlotsWidget->GetCachedGeometry().AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	FVector2D ItemWidgetPos = SlotsWidget->GetCachedGeometry().AbsoluteToLocal(InGeometry.LocalToAbsolute(InventorySlotSize / 2.f));
	FIntPoint ItemSlotPos = FIntPoint(ItemWidgetPos.X / InventorySlotSize.X, ItemWidgetPos.Y / InventorySlotSize.Y);

	CachedFromSlotPos = ItemSlotPos;
	CachedDeltaWidgetPos = MouseWidgetPos - ItemWidgetPos;

	return Reply;
}

void US1InventoryItemInfoWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	US1ItemDragWidget* DragWidget = CreateWidget<US1ItemDragWidget>(GetOwningPlayer(), DragWidgetClass);
	FVector2D ItemInfoWidgetSize = FVector2D(1 * 50, 1 * 50);
	
	// ItemInfoWidget�� Brush�� �����ͼ� UTexture2D*�� ��ȯ, DragWidget�� �Ѱ���
	FSlateBrush Brush = Icon_Image->Brush;
	UTexture2D* ItemIcon = Cast<UTexture2D>(Brush.GetResourceObject());
	DragWidget->Init(ItemInfoWidgetSize, ItemIcon, ItemCount);

	US1DragDropOperation* DragDrop = NewObject<US1DragDropOperation>();
	DragDrop->DefaultDragVisual = DragWidget;
	DragDrop->Pivot = EDragPivot::MouseDown;
	DragDrop->FromItemSlotPos = CachedFromSlotPos;
	DragDrop->ItemInstance = ItemInstance;
	DragDrop->DeltaWidgetPos = CachedDeltaWidgetPos;
	
	OutOperation = DragDrop;
}

void US1InventoryItemInfoWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);
	//UE_LOG(LogTemp, Warning, TEXT("NativeOnDragCancelled"));
	// �������� �巡���� ��, �ùٸ��� ���� ��ġ�� ���� �� -> ĵ��
	
	//RefreshWidgetOpacity(true);
}

void US1InventoryItemInfoWidget::RefreshWidgetOpacity(bool bClearlyVisible)
{
	SetRenderOpacity(bClearlyVisible ? 1.f : 0.5f);
}

void US1InventoryItemInfoWidget::RefreshItemCount(int32 NewItemCount)
{
	ItemCount = NewItemCount;
	Count_Text->SetText((ItemCount >= 2) ? FText::AsNumber(ItemCount) : FText::GetEmpty());
}