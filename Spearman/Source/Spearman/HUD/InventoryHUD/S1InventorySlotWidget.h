// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Spearman/HUD/S1UserWidget.h"
#include "S1InventorySlotWidget.generated.h"

class USizeBox;
class UImage;


UCLASS()
class SPEARMAN_API US1InventorySlotWidget : public US1UserWidget
{
	GENERATED_BODY()
	
public:
	US1InventorySlotWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(meta = (BindWidget))
	USizeBox* Root_SizeBox;

	UPROPERTY(meta = (BindWidget))
	UImage* Slot_Image;
	
};
