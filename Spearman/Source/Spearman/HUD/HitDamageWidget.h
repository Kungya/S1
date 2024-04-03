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
	
private:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HitDamageText;
};
