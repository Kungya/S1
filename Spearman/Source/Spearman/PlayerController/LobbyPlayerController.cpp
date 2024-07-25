// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyPlayerController.h"
#include "Spearman/HUD/LobbyHUD.h"
#include "Spearman/HUD/LobbyOverlay.h"


void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	LobbyHUD = Cast<ALobbyHUD>(GetHUD());
}

void ALobbyPlayerController::Tick(float DeltaTime)
{
	if (IsLocalController())
	{
		HUDInit();
	}
}

void ALobbyPlayerController::HUDInit()
{
	if (LobbyOverlay == nullptr)
	{
		LobbyHUD = (LobbyHUD == nullptr) ? Cast<ALobbyHUD>(GetHUD()) : LobbyHUD;

		if (LobbyHUD)
		{
			LobbyHUD->AddLobbyOverlay();
			
			if (LobbyHUD->LobbyOverlay)
			{
				LobbyOverlay = LobbyHUD->LobbyOverlay;
			}
		}
	}
}
