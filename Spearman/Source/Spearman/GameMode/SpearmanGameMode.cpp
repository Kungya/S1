// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanGameMode.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Spearman/GameMode/BlueZone.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Spearman/SpearComponents/InventoryComponent.h"

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
		{ /* Seamless Travel */
			bUseSeamlessTravel = true;
//# if WITH_EDITOR
//			bUseSeamlessTravel = false;
//# endif
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
{ /* PlayerController Num is 1 when MatchState::WaitingToStart (Only Server) */
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		ASpearmanPlayerController* SpearmanPlayerController = Cast<ASpearmanPlayerController>(*It);
		if (SpearmanPlayerController)
		{
			if (MatchState == MatchState::Cooldown)
			{ // EmptyInventory if PlayerController failed to Extract
				if (!WinnerList.Contains(SpearmanPlayerController))
				{
					SpearmanPlayerController->GetSpearmanCharacter()->Death();
					SpearmanPlayerController->GetInventory()->EmptyInventory();
				}
			}

			SpearmanPlayerController->OnMatchStateSet(MatchState);
		}
	}

	if (MatchState == MatchState::Cooldown)
	{
		WinnerList.Empty();
	}
}

void ASpearmanGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
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

void ASpearmanGameMode::HandleSeamlessTravelPlayer(AController*& C)
{ /* Called After SeamlessTravel, Old HUD was Destroyed and new HUD is valid */
	Super::HandleSeamlessTravelPlayer(C);

	ASpearmanPlayerController* SpearmanPlayerController = Cast<ASpearmanPlayerController>(C);
	if (SpearmanPlayerController)
	{
		SpearmanPlayerController->ClientHandleSeamlessTravelPlayer();
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