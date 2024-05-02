// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "S1UserWidget.h"
#include "S1InventorySlotWidget.generated.h"

/**
 * 
 */
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
	class USizeBox* Root_SizeBox;

	UPROPERTY(meta = (BindWidget))
	class UImage* Slot_Image;
	
};
