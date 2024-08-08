// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Spearman/HUD/S1UserWidget.h"
#include "S1ItemDragWidget.generated.h"

class USizeBox;
class UImage;
class UTextBlock;


UCLASS()
class SPEARMAN_API US1ItemDragWidget : public US1UserWidget
{
	GENERATED_BODY()
	
public:
	US1ItemDragWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	void Init(const FVector2D InWidgetSize, UTexture2D* InItemIcon, int32 InItemCount);

protected:
	UPROPERTY(meta = (BindWidget))
	USizeBox* Root_SizeBox;
	 
	UPROPERTY(meta = (BindWidget))
	UImage* Icon_Image;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Count_Text;
};
