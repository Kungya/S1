// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanHUD.h"
#include "CharacterOverlay.h"
#include "CharacterOverlayNotice.h"
#include "CharacterOverlayCooldown.h"
#include "ExtractionNoticeWidget.h"


void ASpearmanHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ASpearmanHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void ASpearmanHUD::AddCharacterOverlayNotice()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayNoticeClass)
	{
		CharacterOverlayNotice = CreateWidget<UCharacterOverlayNotice>(PlayerController, CharacterOverlayNoticeClass);
		
		if (CharacterOverlayNotice)
		{
			CharacterOverlayNotice->AddToViewport();
		}
	}
}

void ASpearmanHUD::AddCharacterOverlayCooldown()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayCooldownClass)
	{
		CharacterOverlayCooldown = CreateWidget<UCharacterOverlayCooldown>(PlayerController, CharacterOverlayCooldownClass);

		if (CharacterOverlayCooldown)
		{
			CharacterOverlayCooldown->AddToViewport();
		}
	}
}

void ASpearmanHUD::AddExtractionNoticeWidget(float SingleTripTime)
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && ExtractionNoticeWidgetClass && ExtractionNoticeWidget == nullptr)
	{
		ExtractionNoticeWidget = CreateWidget<UExtractionNoticeWidget>(PlayerController, ExtractionNoticeWidgetClass);
	}

	ExtractionNoticeWidget->AddToViewport();
	ExtractionNoticeWidget->PlayExtractionProgressAnimation(SingleTripTime);
}

void ASpearmanHUD::OnHUDStateChanged(EHUDState NewHUDState)
{
	HUDState = NewHUDState;
}

void ASpearmanHUD::Destroyed()
{
	Super::Destroyed();
}

void ASpearmanHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D CenterInViewport(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

		if (HUDPackage.CrosshairCircle)
			DrawCrosshair(HUDPackage.CrosshairCircle, CenterInViewport);
		if (HUDPackage.CrosshairDot)
			DrawCrosshair(HUDPackage.CrosshairDot, CenterInViewport);
	}
}

void ASpearmanHUD::DrawCrosshair(UTexture2D* Texture, FVector2D CenterInViewport)
{
	const float Width = Texture->GetSizeX();
	const float Height = Texture->GetSizeY();
	
	const FVector2D CorrectedCenter(CenterInViewport.X - Width / 2.f, CenterInViewport.Y - Height / 2.f);

	AHUD::DrawTexture(Texture, CorrectedCenter.X, CorrectedCenter.Y, Width, Height, 0.f, 0.f, 1.f, 1.f, FLinearColor::Black);
}
