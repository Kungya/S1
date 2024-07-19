// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SpearmanHUD.generated.h"

class UUserWidget;
class UCharacterOverlay;
class UCharacterOverlayNotice;
class UTexture2D;

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
	
	UTexture2D* CrosshairCircle;
	UTexture2D* CrosshairDot;
};

UENUM(BlueprintType)
enum class EHUDState : uint8
{
	EHS_Playing UMETA(DisplayName = "Playing"),
	EHS_Spectating UMETA(DisplayName = "Spectating"),
	EHS_Inactive UMETA(DisplayName = "Inactive"),

	EHS_MAX UMETA(DisplayName = "DefaultMAX")
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
	TSubclassOf<UUserWidget> CharacterOverlayClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> CharacterOverlayNoticeClass;

	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;

	UPROPERTY()
	UCharacterOverlayNotice* CharacterOverlayNotice;
	
	void AddCharacterOverlay();
	void AddCharacterOverlayNotice();

	UFUNCTION(Category = "HUDEvents")
	void OnHUDStateChanged(EHUDState NewHUDState);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere)
	FHUDPackage HUDPackage;

	UPROPERTY()
	EHUDState HUDState;

	void DrawCrosshair(UTexture2D* Texture, FVector2D CenterInViewport);
public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
	FORCEINLINE EHUDState GetHUDState() const { return HUDState; }
};
