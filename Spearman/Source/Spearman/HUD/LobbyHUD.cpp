// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyHUD.h"
#include "LobbyOverlay.h"

void ALobbyHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ALobbyHUD::AddLobbyOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && LobbyOverlayClass)
	{
		LobbyOverlay = CreateWidget<ULobbyOverlay>(PlayerController, LobbyOverlayClass);
		LobbyOverlay->AddToViewport();
	}
}

