// Fill out your copyright notice in the Description page of Project Settings.


#include "S1SpectatorPawn.h"
#include "GameFramework/PlayerController.h"

void AS1SpectatorPawn::SetupPlayerInputComponent(UInputComponent* InInputComponent)
{
	Super::SetupPlayerInputComponent(InInputComponent);

	InInputComponent->BindAction("ViewPrev", IE_Pressed, this, &AS1SpectatorPawn::ViewPrevPlayer);
	InInputComponent->BindAction("ViewNext", IE_Pressed, this, &AS1SpectatorPawn::ViewNextPlayer);
}

void AS1SpectatorPawn::ViewPrevPlayer()
{
	APlayerController* PlayerController = GetController<APlayerController>();
	if (PlayerController)
	{
		PlayerController->ServerViewPrevPlayer();
	}
}

void AS1SpectatorPawn::ViewNextPlayer()
{
	APlayerController* PlayerController = GetController<APlayerController>();
	if (PlayerController)
	{
		PlayerController->ServerViewNextPlayer();
	}
}
