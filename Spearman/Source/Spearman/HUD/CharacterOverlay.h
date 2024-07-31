// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "S1UserWidget.h"
#include "CharacterOverlay.generated.h"

class UCanvasPanel;
class UProgressBar;
class UTextBlock;
class UImage;
class US1InventoryWidget;
class US1InventorySlotsWidget;
class UItemDropCanvasWidget;
class UItemSaleWidget;


UCLASS()
class SPEARMAN_API UCharacterOverlay : public US1UserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
public:
	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* Root_CanvasPanel;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HpBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HpText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountdownText;

	UPROPERTY(meta = (BindWidget))
	US1InventoryWidget* InventoryWidget;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Ping_Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ClientTick_Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ServerTick_Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Alive_Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Richest_Text;

	UPROPERTY(meta = (BindWidget))
	UImage* BlueZoneImage;

	UPROPERTY(meta = (BindWidget))
	UImage* Minimap;

	UPROPERTY(meta = (BindWidget))
	UItemSaleWidget* ItemSaleWidget;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ExtractionNoticeText;
};
