// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SpearmanGameMode.generated.h"

namespace MatchState
{
	extern SPEARMAN_API const FName Cooldown;

}

/**
 * 
 */
UCLASS()
class SPEARMAN_API ASpearmanGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ASpearmanGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerDeath(class ASpearmanCharacter* DeadCharacter, class ASpearmanPlayerController* VictimController, ASpearmanPlayerController* AttackerController);
	virtual void RequestRespawn(ASpearmanCharacter* DeadCharacter, AController* DeadController);

	float BeginPlayTime = 0.f;

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;
	
	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;
protected:
	virtual void BeginPlay() override;

	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0.f;

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
