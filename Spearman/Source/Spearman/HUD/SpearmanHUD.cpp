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
{ // -> DrawHUD�� ������ �� �����Ӹ��� �ڵ����� call�� ���ֱ� ������ override�ؼ� �ڵ带 �־��ֱ⸸ �ϸ� �ȴ�
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
	
	// 1/2�� �о �߾ӿ� ����  
	const FVector2D CorrectedCenter(CenterInViewport.X - Width / 2.f, CenterInViewport.Y - Height / 2.f);

	AHUD::DrawTexture(Texture, CorrectedCenter.X, CorrectedCenter.Y, Width, Height, 0.f, 0.f, 1.f, 1.f, FLinearColor::Black);
}
