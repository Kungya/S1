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


void ASpearmanPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SpearmanHUD = Cast<ASpearmanHUD>(GetHUD());

	if (SpearmanHUD)
	{
		SpearmanHUD->AddCharacterOverlayNotice();
	}
}

void ASpearmanPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	HUDInit();

	DeltaTimeSumforTimeSync += DeltaTime;
	if (IsLocalController() && DeltaTimeSumforTimeSync >= 5.f)
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

void ASpearmanPlayerController::SetHUDHp(float Hp, float MaxHp)
{
	SpearmanHUD = SpearmanHUD == nullptr ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;

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
		bInitializeCharacterOverlay = true;
		HUDHp = Hp;
		HUDMaxHp = MaxHp;
	}
}

void ASpearmanPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	SpearmanHUD = SpearmanHUD == nullptr ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;

	if (SpearmanHUD && SpearmanHUD->CharacterOverlay)
	{
		if (SpearmanHUD->CharacterOverlay->MatchCountdownText)
		{
			int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
			int32 Seconds = CountdownTime  - Minutes * 60;

			FString MatchCountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
			SpearmanHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(MatchCountdownText));
		}
	}
}

void ASpearmanPlayerController::SetHUDTime()
{
	uint32 LeftTime = FMath::CeilToInt(MatchTime - GetServerTime());

	if (CountdownInt != LeftTime)
	{ // 초 단위가 변경되었음
		SetHUDMatchCountdown(MatchTime - GetServerTime());
	}
	 
	CountdownInt = LeftTime;
}

void ASpearmanPlayerController::HUDInit()
{ // BeginPlay에서도 CharacterOveraly가 null일 경우가 있어, Tick에서 대기해서 처리
	if (CharacterOverlay == nullptr)
	{
		if (SpearmanHUD && SpearmanHUD->CharacterOverlay)
		{
			CharacterOverlay = SpearmanHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHp(HUDHp, HUDMaxHp);
			}
		}
	}
}

void ASpearmanPlayerController::ServerRequestServerTime_Implementation(float ClientRequestTime)
{ // server only
	const float ServerTime = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(ClientRequestTime, ServerTime);
}

void ASpearmanPlayerController::ClientReportServerTime_Implementation(float ClientRequestTime, float ServerReportTime)
{ // CurrentServerTime = ServerTime + 1/2RTT;
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - ClientRequestTime;
	const float CurrentServerTime = ServerReportTime + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
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

void ASpearmanPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ASpearmanPlayerController::OnMatchStateSet(FName State)
{ // Server Only
	MatchState = State;

	if (MatchState == MatchState::WaitingToStart)
	{
		// TODO : 
	}

	if (MatchState == MatchState::InProgress)
	{
		SpearmanHUD = SpearmanHUD == nullptr ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;
		if (SpearmanHUD)
		{
			SpearmanHUD->AddCharacterOverlay();

			if (SpearmanHUD->CharacterOverlayNotice)
			{
				SpearmanHUD->CharacterOverlayNotice->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void ASpearmanPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		SpearmanHUD = SpearmanHUD == nullptr ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;
		if (SpearmanHUD)
		{ // AddCharacterOverlay when Mtach Starts (Hp, MatchTime....and so on)
			SpearmanHUD->AddCharacterOverlay();

			if (SpearmanHUD->CharacterOverlayNotice)
			{
				SpearmanHUD->CharacterOverlayNotice->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void ASpearmanPlayerController::ServerCheckMatchState_Implementation()
{ // server only 
	ASpearmanGameMode* GameMode = Cast<ASpearmanGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		BeginPlayTime = GameMode->BeginPlayTime;
		MatchState = GameMode->GetMatchState();
	}
}