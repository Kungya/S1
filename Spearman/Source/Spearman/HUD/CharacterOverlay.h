// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "S1UserWidget.h"
//#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

class US1InventoryWidget;
class US1InventorySlotsWidget;

/**
 * 
 */
UCLASS()
class SPEARMAN_API UCharacterOverlay : public US1UserWidget
{
	GENERATED_BODY()
	
public:

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<US1InventoryWidget> InventoryWidgetClass;

private:
	// TODO : move to InventoryWidget
	FVector2D InventoryWidgetPosition = FVector2D(-600.f, -475.f);
	FVector2D InventoryWidgetSize = FVector2D(550.f, 950.f);
	
public:
	UPROPERTY(meta = (BindWidget))
	class UCanvasPanel* Root_CanvasPanel;

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HpBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HpText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountdownText;

	UPROPERTY(meta = (BindWidget))
	US1InventoryWidget* InventoryWidget;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Ping_Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ServerTick_Text;
};
