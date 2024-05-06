// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "S1UserWidget.h"
#include "S1InventoryWidget.generated.h"

class UCanvasPanel;
class US1InventorySlotsWidget;
class ASpearmanPlayerController;

/**
 * 
 */
UCLASS()
class SPEARMAN_API US1InventoryWidget : public US1UserWidget
{
	GENERATED_BODY()
	
public:

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<US1InventorySlotsWidget> SlotsWidgetClass;
	
private:
	/* Caching Owner PlayerController */
	ASpearmanPlayerController* SpearmanPlayerController;

public:
	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* Item_CanvasPanel;

	UPROPERTY(meta = (BindWidget))
	US1InventorySlotsWidget* InventorySlotsWidget;
};
