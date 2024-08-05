// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SpearmanPlayerController.generated.h"

class ASpearmanHUD;
class ASpearmanGameMode;
class UCharacterOverlay;
class ASpearmanCharacter;
class USceneCaptureComponent2D;
class UReturnToMainMenu;
class UExtractionResultSubsystem;
class US1InventoryWidget;
class UInventoryComponent;
class US1InventorySlotsWidget;

UCLASS()
class SPEARMAN_API ASpearmanPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	ASpearmanPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void OnPossess(APawn* InPawn) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetHUDHp(float Hp, float MaxHp);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDNoticeCountdown(float CountdownTime);
	void SetHUDCooldownCountdown(float CountdownTime);
	void SetHUDBalance(int32 Balance);
	void SetHUDBalanceRanking();

	virtual float GetServerTime();

	UFUNCTION(Client, Reliable)
	void ClientHandleSeamlessTravelPlayer();

	virtual void ReceivedPlayer() override;
	void OnMatchStateSet(FName State);

	void HandleWaitingToStart();
	void HandleMatchHasStarted();
	void HandleCooldown();

	void HandleExtraction();
	void ExtractionCallback();

	void ShowInventoryWidget();

	UFUNCTION()
	void SetPlayerPlay();
	
	UFUNCTION()
	void SetPlayerSpectate();

	UFUNCTION(Client, Unreliable)
	void ClientSetExtractionNotice();

	UFUNCTION(Client, Unreliable)
	void ClientClearExtractionNotice();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	void SetHUDTime();
	void HUDInit();
	void SetHUDPing(float DeltaTime);
	void SetHUDTickRate(float ClientTick, float ServerTick);
	void SetHUDAlive();

	void InitRenderTargetIfServer(APawn* InPawn);

	void ShowReturnToMainMenu();

	/* Sync time */

	FTimerHandle RequestServerTimeHandle;

	void RequestServerTime();

	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float ClientRequestTime);

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float ClientRequestTime, float ServerReportTime, float ServerReportTickRate);
	
	float ClientServerDelta = 0.f;
	float SingleTripTime = 0.f;

	UFUNCTION(Server, Reliable)
	void ServerRequestMatchState();

	UFUNCTION(Client, Reliable)
	void ClientReportMatchState(FName ServerMatchState, float ServerBeginPlayTime, float ServerWarmupTime, float ServerMatchTime, float ServerCooldownTime);
	
	/* Spectator */
	
	UFUNCTION(Client, Reliable)
	void ClientHUDStateChanged(EHUDState NewState);
	
	virtual void OnRep_Pawn() override;

	/* Extraction HUD */

	UFUNCTION(Client, Reliable)
	void ClientEnableItemSale();

	UFUNCTION(Client, Unreliable)
	void ClientSetSpectatorHUD();

	virtual void NotifyLoadedWorld(FName WorldPackageName, bool bFinalDest) override;

private:
	// Client should get MatchTime from Server
	float BeginPlayTime = 0.f;
	float WarmupTime = 0.f;
	float MatchTime = 0.f;
	float CooldownTime = 0.f;
	float TimeStampWaitingToStart = 0.f;

	uint32 CountdownInt = 0;

	// tracking current MatchState from GameMode
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	ASpearmanCharacter* SpearmanCharacter;

	UPROPERTY()
	ASpearmanGameMode* SpearmanGameMode;

	UPROPERTY()
	ASpearmanHUD* SpearmanHUD;

	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;
	
	float HUDHp;
	float HUDMaxHp;

	float PingCheckTime = 0.f;
	float LocalTickRate = 0.f;
	float ServerTickRate = 0.f;

	int32 AliveCount = 0;
	int32 SpectatorCount = 0;

	/* ReturnToMainMenu */
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> ReturnToMainMenuWidget;
	
	UPROPERTY()
	UReturnToMainMenu* ReturnToMainMenu;

	bool bReturnToMainMenuOpen = false;

	UPROPERTY(VisibleAnywhere, Category = "ActorComponent")
	UInventoryComponent* Inventory;

	float TestDeltaTimeSum = 0.f;

public:
	FORCEINLINE ASpearmanHUD* GetSpearmanHUD() const { return SpearmanHUD;  }
	FORCEINLINE float GetSingleTripTime() const { return SingleTripTime; }
	FORCEINLINE UInventoryComponent* GetInventory() const { return Inventory; }
	FORCEINLINE ASpearmanCharacter* GetSpearmanCharacter() const { return SpearmanCharacter; }
};
