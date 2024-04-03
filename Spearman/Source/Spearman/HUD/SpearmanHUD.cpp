// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"

void ASpearmanHUD::BeginPlay()
{
	Super::BeginPlay();

	AddCharacterOverlay();
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

void ASpearmanHUD::DrawHUD()
{ // -> DrawHUD는 엔진이 매 프레임마다 자동으로 call을 해주기 때문에 override해서 코드를 넣어주기만 하면 된다
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
	
	// 1/2씩 밀어서 중앙에 정렬  
	const FVector2D CorrectedCenter(CenterInViewport.X - Width / 2.f, CenterInViewport.Y - Height / 2.f);

	AHUD::DrawTexture(Texture, CorrectedCenter.X, CorrectedCenter.Y, Width, Height, 0.f, 0.f, 1.f, 1.f, FLinearColor::Black);
}
