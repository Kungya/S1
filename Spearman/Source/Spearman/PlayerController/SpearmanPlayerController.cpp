// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanPlayerController.h"
#include "Spearman/HUD/SpearmanHUD.h"
#include "Spearman/HUD/CharacterOverlay.h"
#include "Spearman/HUD/CharacterOverlayNotice.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/GameMode/SpearmanGameMode.h"
#include "Components/Progressbar.h"
#include "Components/TextBlock.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Spearman/HUD/S1InventoryWidget.h"
#include "GameFramework/PlayerState.h"


void ASpearmanPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SpearmanHUD = Cast<ASpearmanHUD>(GetHUD());

	// Client should get MatchState from Server asap.
	ServerRequestMatchState();
}

void ASpearmanPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	LocalTickRate = 1.f / DeltaTime;

	if (IsLocalController())
	{
		SetHUDTickRate(LocalTickRate, ServerTickRate);
		SetHUDTime();
		SetHUDPing(DeltaTime);

		HUDInit();
	}

	DeltaTimeSumforTimeSync += DeltaTime;
	if (IsLocalController() && DeltaTimeSumforTimeSync > 5.f)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		DeltaTimeSumforTimeSync = 0.f;
	}
}

void ASpearmanPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpearmanPlayerController, MatchState);
}

void ASpearmanPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ASpearmanCharacter* SpearmanCharacter = Cast<ASpearmanCharacter>(InPawn);
	if (SpearmanCharacter)
	{
		SetHUDHp(SpearmanCharacter->GetHp(), SpearmanCharacter->GetMaxHp());
	}
}

void ASpearmanPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ASpearmanPlayerController::SetHUDHp(float Hp, float MaxHp)
{
	SpearmanHUD = (SpearmanHUD == nullptr) ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;

	if (SpearmanHUD && SpearmanHUD->CharacterOverlay)
	{
		if (SpearmanHUD->CharacterOverlay->HpBar && SpearmanHUD->CharacterOverlay->HpText)
		{
			const float HpPercent = Hp / MaxHp;
			SpearmanHUD->CharacterOverlay->HpBar->SetPercent(HpPercent);
			
			FString HpText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Hp), FMath::CeilToInt(MaxHp));
			SpearmanHUD->CharacterOverlay->HpText->SetText(FText::FromString(HpText));
		}
	}
	else
	{
		HUDHp = Hp;
		HUDMaxHp = MaxHp;
	}
}

void ASpearmanPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	SpearmanHUD = (SpearmanHUD == nullptr) ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;

	if (SpearmanHUD && SpearmanHUD->CharacterOverlay)
	{
		if (SpearmanHUD->CharacterOverlay->MatchCountdownText)
		{
			if (CountdownTime < 0.f)
			{
				SpearmanHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
				return;
			}

			int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
			int32 Seconds = CountdownTime  - Minutes * 60;

			FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
			SpearmanHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
		}
	}
}

void ASpearmanPlayerController::SetHUDNoticeCountdown(float CountdownTime)
{
	SpearmanHUD = (SpearmanHUD == nullptr) ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;

	if (SpearmanHUD && SpearmanHUD->CharacterOverlayNotice)
	{
		if (SpearmanHUD->CharacterOverlayNotice->WarmupTimeText)
		{
			if (CountdownTime < 0.f)
			{
				SpearmanHUD->CharacterOverlayNotice->WarmupTimeText->SetText(FText());
				return;
			}

			int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
			int32 Seconds = CountdownTime - Minutes * 60;

			FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
			SpearmanHUD->CharacterOverlayNotice->WarmupTimeText->SetText(FText::FromString(CountdownText));
		}
	}
}

void ASpearmanPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
	{
		// GetServerTime() = BeginplayTime + passed time
		TimeLeft = WarmupTime - GetServerTime() + BeginPlayTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + BeginPlayTime;
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + BeginPlayTime;
	}

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	// TODO : HasAuthority()

	if (CountdownInt != SecondsLeft)
	{ // true per sec
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDNoticeCountdown(TimeLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	 
	CountdownInt = SecondsLeft;
}

void ASpearmanPlayerController::HUDInit()
{ // BeginPlay에서도 CharacterOverlay가 null일 경우가 있어, Tick에서 대기해서 처리
	if (CharacterOverlay == nullptr)
	{
		if (SpearmanHUD && SpearmanHUD->CharacterOverlay)
		{
			CharacterOverlay = SpearmanHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHp(HUDHp, HUDMaxHp);
				CharacterOverlay->InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void ASpearmanPlayerController::SetHUDPing(float DeltaTime)
{
	PingCheckTime += DeltaTime;
	if (PingCheckTime >= 1.f)
	{
		PingCheckTime = 0.f;

		if (SpearmanHUD && CharacterOverlay)
		{
			PlayerState = (PlayerState == nullptr) ? GetPlayerState<APlayerState>() : PlayerState;
			if (PlayerState)
			{
				const float CurrentPing = PlayerState->GetCompressedPing() * 4.f;
				FString PingText = FString::Printf(TEXT("%d ms"), FMath::FloorToInt(CurrentPing));
				CharacterOverlay->Ping_Text->SetText(FText::FromString(PingText));
			}
		}
	}
}

void ASpearmanPlayerController::SetHUDTickRate(float ClientTick, float ServerTick)
{
	SpearmanHUD = (SpearmanHUD == nullptr) ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;
	if (SpearmanHUD && CharacterOverlay)
	{
		FString ClientTickText = FString::Printf(TEXT("%d"), FMath::FloorToInt(ClientTick));
		CharacterOverlay->ClientTick_Text->SetText(FText::FromString(ClientTickText));
		FString ServerTickText = FString::Printf(TEXT("%d"), FMath::FloorToInt(ServerTick));
		CharacterOverlay->ServerTick_Text->SetText(FText::FromString(ServerTickText));
	}
}

void ASpearmanPlayerController::ServerRequestServerTime_Implementation(float ClientRequestTime)
{ /* Server Only */
	const float ServerTime = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(ClientRequestTime, ServerTime, LocalTickRate);
}

void ASpearmanPlayerController::ClientReportServerTime_Implementation(float ClientRequestTime, float ServerReportTime, float ServerReportTickRate)
{ // CurrentServerTime = ServerTime + 1/2RTT;
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - ClientRequestTime;
	SingleTripTime = (0.5f * RoundTripTime);
	const float CurrentServerTime = ServerReportTime + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
	ServerTickRate = ServerReportTickRate;
}

float ASpearmanPlayerController::GetServerTime()
{
	if (HasAuthority())
	{
		return GetWorld()->GetTimeSeconds();
	}
	else
	{
		return GetWorld()->GetTimeSeconds() + ClientServerDelta;
	}
}

void ASpearmanPlayerController::HandleMatchHasStarted()
{
	SpearmanHUD = (SpearmanHUD == nullptr) ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;
	if (SpearmanHUD)
	{ // AddCharacterOverlay when Mtach Starts (Hp, MatchTime....and so on)
		SpearmanHUD->AddCharacterOverlay();

		if (SpearmanHUD->CharacterOverlayNotice)
		{
			SpearmanHUD->CharacterOverlayNotice->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ASpearmanPlayerController::HandleCooldown()
{
	SpearmanHUD = (SpearmanHUD == nullptr) ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;
	if (SpearmanHUD)
	{
		SpearmanHUD->CharacterOverlay->RemoveFromParent();
		if (SpearmanHUD->CharacterOverlayNotice && SpearmanHUD->CharacterOverlayNotice->WarmupNoticeText)
		{
			SpearmanHUD->CharacterOverlayNotice->SetVisibility(ESlateVisibility::Visible);
			FString NoticeText("Match End. New Match Stats Soon !");
			SpearmanHUD->CharacterOverlayNotice->WarmupNoticeText->SetText(FText::FromString(NoticeText));
		}
	}
	ASpearmanCharacter* SpearmanCharacter = Cast<ASpearmanCharacter>(GetPawn());
	if (SpearmanCharacter)
	{
		SpearmanCharacter->bDisableKeyInput = true;
	}
}

void ASpearmanPlayerController::ShowInventoryWidget()
{
	SpearmanHUD = (SpearmanHUD == nullptr) ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;

	if (SpearmanHUD && CharacterOverlay)
	{
		US1InventoryWidget* InventoryWidget = SpearmanHUD->CharacterOverlay->InventoryWidget;
		if (InventoryWidget)
		{
			InventoryWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void ASpearmanPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ASpearmanPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ASpearmanPlayerController::ServerRequestMatchState_Implementation()
{ // server only, Client should get MatchState from Server ASAP. 
	ASpearmanGameMode* GameMode = Cast<ASpearmanGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		MatchState = GameMode->GetMatchState();
		BeginPlayTime = GameMode->BeginPlayTime;
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		ClientReportMatchState(MatchState, BeginPlayTime, WarmupTime, MatchTime, CooldownTime);
	}
}

void ASpearmanPlayerController::ClientReportMatchState_Implementation(FName ServerMatchState, float ServerBeginPlayTime, float ServerWarmupTime, float ServerMatchTime, float ServerCooldownTime)
{ // client only
	MatchState = ServerMatchState;
	BeginPlayTime = ServerBeginPlayTime;
	WarmupTime = ServerWarmupTime;
	MatchTime = ServerMatchTime;
	CooldownTime = ServerCooldownTime;
	OnMatchStateSet(MatchState);
	
	if (SpearmanHUD && MatchState == MatchState::WaitingToStart)
	{
		SpearmanHUD->AddCharacterOverlayNotice();
	}
}