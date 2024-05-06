// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "S1UserWidget.h"
#include "S1InventoryItemInfoWidget.generated.h"

class US1InventorySlotsWidget;
class UItemInstance;
class US1ItemDragWidget;

/**
 * 
 */
UCLASS()
class SPEARMAN_API US1InventoryItemInfoWidget : public US1UserWidget
{
	GENERATED_BODY()
	
public:
	US1InventoryItemInfoWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
public:
	void Init(US1InventorySlotsWidget* InSlotsWidget, UItemInstance* InItemInstance, int32 InItemCount);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	void RefreshWidgetOpacity(bool bClearlyVisible);
	void RefreshItemCount(int32 NewItemCount);

private:
	// Caching Info
	FIntPoint CachedFromSlotPos = FIntPoint::ZeroValue;
	FVector2D CachedDeltaWidgetPos = FVector2D::ZeroVector;
	int32 ItemCount = 0;

protected:
	// Parent
	UPROPERTY()
	US1InventorySlotsWidget* SlotsWidget;

	// Item Info
	UPROPERTY()
	UItemInstance* ItemInstance;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<US1ItemDragWidget> DragWidgetClass;

protected:

	UPROPERTY(meta = (BindWidget))
	class USizeBox* Root_SizeBox;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Count_Text;

	UPROPERTY(meta = (BindWidget))
	class UImage* Icon_Image;

	UPROPERTY(meta = (BindWidget))
	UImage* Hover_Image;

public:
	FORCEINLINE UItemInstance* GetItemInstance() const { return ItemInstance; }
	FORCEINLINE int32 GetItemCount() const { return ItemCount; }
};
