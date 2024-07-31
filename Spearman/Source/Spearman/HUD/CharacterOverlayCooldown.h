// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "S1UserWidget.h"
#include "CharacterOverlayCooldown.generated.h"

class UTextBlock;
class US1InventoryWidget;

UCLASS()
class SPEARMAN_API UCharacterOverlayCooldown : public US1UserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CooldownTimeText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CooldownNoticeText;

	UPROPERTY(meta = (BindWidget))
	US1InventoryWidget* InventoryWidget;
};
