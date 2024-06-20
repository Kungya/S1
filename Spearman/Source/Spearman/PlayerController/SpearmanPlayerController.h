// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SpearmanPlayerController.generated.h"

class ASpearmanHUD;
class ASpearmanGameMode;
class UCharacterOverlay;
class ASpearmanCharacter;

UCLASS()
class SPEARMAN_API ASpearmanPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void SetHUDHp(float Hp, float MaxHp);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDNoticeCountdown(float CountdownTime);
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetServerTime();
	// function that Sync server time ASAP
	virtual void ReceivedPlayer() override;
	void OnMatchStateSet(FName State);
	
	void HandleMatchHasStarted();
	void HandleCooldown();

	void ShowInventoryWidget();

protected:
	virtual void BeginPlay() override;
	void SetHUDTime();
	void HUDInit();
	void SetHUDPing(float DeltaTime);
	
	void SetHUDTickRate(float ClientTick, float ServerTick);

	/*
	* Sync time
	*/

	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float ClientRequestTime);

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float ClientRequestTime, float ServerReportTime, float ServerReportTickRate);

	float ClientServerDelta = 0.f;
	float DeltaTimeSumforTimeSync = 0.f;
	float SingleTripTime;

	UFUNCTION(Server, Reliable)
	void ServerRequestMatchState();

	UFUNCTION(Client, Reliable)
	void ClientReportMatchState(FName ServerMatchState, float ServerBeginPlayTime, float ServerWarmupTime, float ServerMatchTime, float ServerCooldownTime);
	
private:
	UPROPERTY()
	ASpearmanHUD* SpearmanHUD;

	UPROPERTY()
	ASpearmanGameMode* SpearmanGameMode;

	UPROPERTY()
	ASpearmanCharacter* SpearmanCharacter;

	// Client should get MatchTime from Server, not in Client
	float BeginPlayTime = 0.f;
	float WarmupTime = 0.f;
	float MatchTime = 0.f;
	float CooldownTime = 0.f;

	uint32 CountdownInt = 0;

	// tracking current MatchState from GameMode
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;
	
	float HUDHp;
	float HUDMaxHp;

	float PingCheckTime = 0.f;

	float LocalTickRate = 0.f;
	float ServerTickRate = 0.f;


public:
	FORCEINLINE ASpearmanHUD* GetSpearmanHUD() const { return SpearmanHUD;  }
	FORCEINLINE float GetSingleTripTime() const { return SingleTripTime; }
};
