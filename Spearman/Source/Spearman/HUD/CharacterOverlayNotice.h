// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlayNotice.generated.h"

/**
 * 
 */
UCLASS()
class SPEARMAN_API UCharacterOverlayNotice : public UUserWidget
{
	GENERATED_BODY()
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* WarmupNoticeText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WarmupTimeText;
};
