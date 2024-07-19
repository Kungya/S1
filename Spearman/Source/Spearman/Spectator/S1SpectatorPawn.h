// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "S1SpectatorPawn.generated.h"

class APlayerController;


UCLASS()
class SPEARMAN_API AS1SpectatorPawn : public ASpectatorPawn
{
	GENERATED_BODY()
	
public:
	virtual void SetupPlayerInputComponent(UInputComponent* InInputComponent) override;

	void ViewPrevPlayer();
	void ViewNextPlayer();

};
