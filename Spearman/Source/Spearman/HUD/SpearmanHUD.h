// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SpearmanHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
	
	class UTexture2D* CrosshairCircle;
	UTexture2D* CrosshairDot;
};

/**
 * 
 */
UCLASS()
class SPEARMAN_API ASpearmanHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;

	// WBP_CharacterOverlay를 들고 있음
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> CharacterOverlayNoticeClass;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY()
	class UCharacterOverlayNotice* CharacterOverlayNotice;
	
	void AddCharacterOverlay();
	void AddCharacterOverlayNotice();

protected:
	virtual void BeginPlay() override;

private:
	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, FVector2D CenterInViewport);
public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};
