// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HitDamageWidget.generated.h"

/**
 * 
 */
UCLASS()
class SPEARMAN_API UHitDamageWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetHitDamageText(float HitDamage);
	void PlayHitDamageAnimation(bool bHeadShot);
	
private:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HitDamageText;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	class UWidgetAnimation* HitDamageAnimation;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	class UWidgetAnimation* HitDamageHeadShotAnimation;
};
