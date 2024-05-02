// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "S1DragDropOperation.generated.h"

/**
 * 
 */
UCLASS()
class SPEARMAN_API US1DragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()
	
public:
	US1DragDropOperation(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
public:
	// �巡�� �Ǳ� �� ���� ��ġ ��ǥ
	FIntPoint FromItemSlotPos = FIntPoint::ZeroValue;
	FVector2D DeltaWidgetPos = FVector2D::ZeroVector;

public:
	class UItemInstance* ItemInstance;
};
