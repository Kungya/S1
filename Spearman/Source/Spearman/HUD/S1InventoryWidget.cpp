// Fill out your copyright notice in the Description page of Project Settings.


#include "S1InventoryWidget.h"
#include "Spearman/HUD/S1InventorySlotsWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

void US1InventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	/*SlotsWidget = CreateWidget<US1InventorySlotsWidget>(GetOwningPlayer(), SlotsWidgetClass);
	Item_CanvasPanel->AddChildToCanvas(SlotsWidget);

	UCanvasPanelSlot* CanvasPanelSlot = Cast<UCanvasPanelSlot>(SlotsWidget->Slot);
	CanvasPanelSlot->SetAnchors(FAnchors::FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	CanvasPanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CanvasPanelSlot->SetAutoSize(true);*/
}