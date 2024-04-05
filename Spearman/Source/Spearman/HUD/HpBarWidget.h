// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HpBarWidget.generated.h"

/**
 * 
 */
UCLASS()
class SPEARMAN_API UHpBarWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetHpBar(float HpRatio);

private:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HpBar;
};
