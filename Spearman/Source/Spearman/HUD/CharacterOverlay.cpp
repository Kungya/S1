// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterOverlay.h"
#include "Spearman/HUD/S1InventoryWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

void UCharacterOverlay::NativeConstruct()
{
	Super::NativeConstruct();

	/*InventoryWidget = CreateWidget<US1InventoryWidget>(GetOwningPlayer(), InventoryWidgetClass);
	Root_CanvasPanel->AddChildToCanvas(InventoryWidget);

	UCanvasPanelSlot* CanvasPanelSlot = Cast<UCanvasPanelSlot>(InventoryWidget->Slot);
	CanvasPanelSlot->SetPosition(InventoryWidgetPosition);
	CanvasPanelSlot->SetSize(InventoryWidgetSize);
	CanvasPanelSlot->SetAnchors(FAnchors::FAnchors(1.0f, 0.5f, 1.0f, 0.5f));*/
}