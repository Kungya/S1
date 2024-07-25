// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LobbyHUD.generated.h"

class ULobbyOverlay;
class UUserWIdget;

UCLASS()
class SPEARMAN_API ALobbyHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> LobbyOverlayClass;

	UPROPERTY()
	ULobbyOverlay* LobbyOverlay;

	void AddLobbyOverlay();


protected:
	virtual void BeginPlay() override;

};
