// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "S1UserWidget.h"
#include "LobbyOverlay.generated.h"

class UCanvasPanel;
class UTextBlock;
class US1InventoryWidget;


UCLASS()
class SPEARMAN_API ULobbyOverlay : public US1UserWidget
{
	GENERATED_BODY()
	

public:
	
	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* Root_CanvasPanel;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountdownText;

	/*UPROPERTY(meta = (BindWidget))
	US1InventoryWidget* InventoryWidget;*/
	
};
