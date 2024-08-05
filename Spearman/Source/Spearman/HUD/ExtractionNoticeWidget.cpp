// Fill out your copyright notice in the Description page of Project Settings.


#include "ExtractionNoticeWidget.h"

void UExtractionNoticeWidget::PlayExtractionProgressAnimation(float SingleTripTime)
{
	PlayAnimation(ExtractionProgressAnimation, SingleTripTime);
}
