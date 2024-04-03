// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanGameMode.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

void ASpearmanGameMode::PlayerDeath(ASpearmanCharacter* DeadCharacter, ASpearmanPlayerController* DeadController, ASpearmanPlayerController* AttackerController)
{
	if (DeadCharacter)
	{
		DeadCharacter->Death();
	}
}

void ASpearmanGameMode::RequestRespawn(ASpearmanCharacter* DeadCharacter, AController* DeadController)
{
	if (DeadCharacter)
	{
		DeadCharacter->Reset();
		DeadCharacter->Destroy();
	}
	if (DeadController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 SelectedIdx = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(DeadController, PlayerStarts[SelectedIdx]);
	}
}
