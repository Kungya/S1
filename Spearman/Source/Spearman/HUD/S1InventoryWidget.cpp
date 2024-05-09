// Fill out your copyright notice in the Description page of Project Settings.


#include "S1InventoryWidget.h"
#include "Spearman/HUD/S1InventorySlotsWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"

void US1InventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bIsFocusable = true;  
	SetKeyboardFocus();
}

FReply US1InventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == FName("Tab"))
	{
		SetVisibility(ESlateVisibility::Hidden);
		SpearmanPlayerController = (SpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(GetOwningPlayer()) : SpearmanPlayerController;
		if (SpearmanPlayerController)
		{
			SpearmanPlayerController->SetInputMode(FInputModeGameOnly());
		}
	}
	
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
