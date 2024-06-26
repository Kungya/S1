// Fill out your copyright notice in the Description page of Project Settings.


#include "HitDamageWidget.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"

void UHitDamageWidget::SetHitDamageText(float HitDamage)
{
	FString HitDamageString = FString::Printf(TEXT("%d"), FMath::CeilToInt(HitDamage));
	HitDamageText->SetText(FText::FromString(HitDamageString));
}

void UHitDamageWidget::PlayHitDamageAnimation(bool bHeadShot)
{
	if (bHeadShot)
	{
		PlayAnimation(HitDamageHeadShotAnimation);
	}
	else
	{
		PlayAnimation(HitDamageAnimation);
	}
}
