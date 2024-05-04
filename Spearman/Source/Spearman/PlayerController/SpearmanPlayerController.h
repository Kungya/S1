// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SpearmanPlayerController.generated.h"

/**
 * 
 */
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
protected:
	virtual void BeginPlay() override;
	void SetHUDTime();
	void HUDInit();
	/*
	* Sync time
	*/

	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float ClientRequestTime);

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float ClientRequestTime, float ServerReportTime);

	float ClientServerDelta = 0.f;

	float DeltaTimeSumforTimeSync = 0.f;

	UFUNCTION(Server, Reliable)
	void ServerRequestMatchState();

	UFUNCTION(Client, Reliable)
	void ClientReportMatchState(FName ServerMatchState, float ServerBeginPlayTime, float ServerWarmupTime, float ServerMatchTime, float ServerCooldownTime);
	
private:
	UPROPERTY()
	class ASpearmanHUD* SpearmanHUD;

	UPROPERTY()
	class ASpearmanGameMode* SpearmanGameMode;

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
	class UCharacterOverlay* CharacterOverlay;

	// TODO : 
	bool bInitializeCharacterOverlay = false;
	
	float HUDHp;
	float HUDMaxHp;

public:
	FORCEINLINE ASpearmanHUD* GetSpearmanHUD() const { return SpearmanHUD;  }
};
