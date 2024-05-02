// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "S1UserWidget.h"
#include "S1ItemDragWidget.generated.h"

/**
 * 
 */
UCLASS()
class SPEARMAN_API US1ItemDragWidget : public US1UserWidget
{
	GENERATED_BODY()
	
public:
	US1ItemDragWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
public:
	void Init(const FVector2D InWidgetSize, UTexture2D* InItemIcon, int32 InItemCount);

protected:
	UPROPERTY(meta = (BindWidget))
	class USizeBox* Root_SizeBox;

	UPROPERTY(meta = (BindWidget))
	class UImage* Icon_Image;

	// TODO : ... 
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Count_Text;
};
