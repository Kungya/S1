// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "S1DragDropOperation.generated.h"

class UItemInstance;
class US1InventorySlotsWidget;
class US1InventoryItemInfoWidget;

UCLASS()
class SPEARMAN_API US1DragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()
	
public:
	US1DragDropOperation(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
public:
	/* Original Pos before Drag */
	FIntPoint FromItemSlotPos = FIntPoint::ZeroValue;
	FVector2D DeltaWidgetPos = FVector2D::ZeroVector;

public:
	UPROPERTY()
	UItemInstance* ItemInstance;
	UPROPERTY()
	US1InventorySlotsWidget* InventorySlotsWidget;

};
