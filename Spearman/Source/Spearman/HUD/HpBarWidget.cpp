// Fill out your copyright notice in the Description page of Project Settings.


#include "HpBarWidget.h"
#include "Components/ProgressBar.h"

void UHpBarWidget::SetHpBar(float HpRatio)
{
	HpBar->SetPercent(HpRatio);
}
