// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanGameMode.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

ASpearmanGameMode::ASpearmanGameMode()
{
	bDelayedStart = true;
}

void ASpearmanGameMode::BeginPlay()
{
	Super::BeginPlay();

	// GameMode is only used in the actually Play Map (not in menu)
	BeginPlayTime = GetWorld()->GetTimeSeconds();
}

void ASpearmanGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		ASpearmanPlayerController* SpearmanPlayerController = Cast<ASpearmanPlayerController>(*It);
		if (SpearmanPlayerController)
		{
			SpearmanPlayerController->OnMatchStateSet(MatchState);
		}
	}
}

void ASpearmanGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		WarmupTime = 10.f - GetWorld()->GetTimeSeconds() + BeginPlayTime;
		
		if (WarmupTime <= 0.f)
		{
			StartMatch();
		}
	}
}

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