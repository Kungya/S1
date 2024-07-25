// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

class ALobbyHUD;
class ULobbyOverlay;

UCLASS()
class SPEARMAN_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	
public:



protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void HUDInit();

private:

	UPROPERTY()
	ALobbyHUD* LobbyHUD;

	UPROPERTY()
	ULobbyOverlay* LobbyOverlay;

public:

};
