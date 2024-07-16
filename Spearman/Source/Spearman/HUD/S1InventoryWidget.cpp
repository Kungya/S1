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
		SpearmanPlayerController = (SpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(GetOwningPlayer()) : SpearmanPlayerController;
		if (SpearmanPlayerController)
		{
			SetVisibility(ESlateVisibility::Hidden);
			FInputModeGameOnly InputModeData;
			SpearmanPlayerController->SetInputMode(InputModeData);
			SpearmanPlayerController->SetShowMouseCursor(false);
		}
	}
	
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
