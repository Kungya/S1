// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SpearmanGameMode.generated.h"

/**
 * 
 */
UCLASS()
class SPEARMAN_API ASpearmanGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	virtual void PlayerDeath(class ASpearmanCharacter* DeadCharacter, class ASpearmanPlayerController* VictimController, ASpearmanPlayerController* AttackerController);

	virtual void RequestRespawn(ASpearmanCharacter* DeadCharacter, AController* DeadController);
};
