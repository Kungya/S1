// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SpearmanGameMode.generated.h"

class ASpearmanCharacter;
class ASpearmanPlayerController;
class ABlueZone;

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
	virtual void PlayerDeath(ASpearmanCharacter* DeadCharacter, ASpearmanPlayerController* VictimController, ASpearmanPlayerController* AttackerController);
	virtual void RequestRespawn(ASpearmanCharacter* DeadCharacter, AController* DeadController);

	void SpawnBlueZone();

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
	virtual void HandleMatchHasStarted() override;

	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	float CountdownTime = 0.f;

	UPROPERTY(EditAnywhere, Category = "BlueZone")
	TSubclassOf<ABlueZone> BlueZoneClass;
	
	UPROPERTY()
	ABlueZone* BlueZone;

	UPROPERTY(EditAnywhere, Category = "BlueZone")
	FTransform BlueZoneTransform;

	UPROPERTY(EditAnywhere, Category = "BlueZone")
	float BlueZoneSpawnDelay = 3.f;

	UPROPERTY(EditAnywhere, Category = "BlueZone")
	bool bActivateBlueZone;

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
