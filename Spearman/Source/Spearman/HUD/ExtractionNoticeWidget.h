// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "S1UserWidget.h"
#include "ExtractionNoticeWidget.generated.h"

class UProgressBar;
class UWidgetAnimation;

UCLASS()
class SPEARMAN_API UExtractionNoticeWidget : public US1UserWidget
{
	GENERATED_BODY()
	
public:
	void PlayExtractionProgressAnimation(float SingleTripTime);
	

private:

	UPROPERTY(meta = (BindWidget))
	UProgressBar* Extraction_ProgressBar;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* ExtractionProgressAnimation;
};
