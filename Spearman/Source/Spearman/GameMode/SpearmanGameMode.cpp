// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanGameMode.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Spearman/GameMode/BlueZone.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

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

void ASpearmanGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + BeginPlayTime;

		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + BeginPlayTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = WarmupTime + MatchTime + CooldownTime - GetWorld()->GetTimeSeconds() + BeginPlayTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
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

void ASpearmanGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ASpearmanPlayerController* NewSpearmanPlayer = Cast<ASpearmanPlayerController>(NewPlayer);
	if (NewSpearmanPlayer)
	{
		SpearmanPlayerControllerArray.AddUnique(NewSpearmanPlayer);
	}
}

void ASpearmanGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if (bActivateBlueZone)
	{
		FTimerHandle SpawnBlueZoneTimer;
		GetWorld()->GetTimerManager().SetTimer(SpawnBlueZoneTimer, this, &ASpearmanGameMode::SpawnBlueZone, BlueZoneSpawnDelay, false);
	}
}

void ASpearmanGameMode::SpawnBlueZone()
{
	BlueZone = GetWorld()->SpawnActor<ABlueZone>(BlueZoneClass, BlueZoneTransform);
	
	if (BlueZone)
	{
		BlueZone->StartMovingBlueZone();
	}
}