// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanPlayerController.h"
#include "Spearman/HUD/SpearmanHUD.h"
#include "Spearman/HUD/CharacterOverlay.h"
#include "Spearman/HUD/CharacterOverlayNotice.h"
#include "Spearman/HUD/CharacterOverlayCooldown.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/GameMode/SpearmanGameMode.h"
#include "Components/Progressbar.h"
#include "Components/TextBlock.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Spearman/HUD/S1InventoryWidget.h"
#include "GameFramework/PlayerState.h"
#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/HUD/ReturnToMainMenu.h"
#include "Spearman/GameInstance/ExtractionResultSubsystem.h"
#include "Spearman/SpearComponents/InventoryComponent.h"
#include "Spearman/HUD/S1InventorySlotsWidget.h"
#include "Spearman/HUD/ItemSaleWidget.h"
#include "Spearman/PlayerState/SpearmanPlayerState.h"
#include "GameFramework/GameStateBase.h"

ASpearmanPlayerController::ASpearmanPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	Inventory->SetIsReplicated(true);
}

void ASpearmanPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SpearmanHUD = Cast<ASpearmanHUD>(GetHUD());
	SpearmanCharacter = Cast<ASpearmanCharacter>(GetPawn());

	// Client should get MatchState from Server asap.
	ServerRequestMatchState();

	if (IsLocalController())
	{
		GetWorldTimerManager().SetTimer(RequestServerTimeHandle, this, &ASpearmanPlayerController::RequestServerTime, 3.f, true, 6.f);
	}

	PlayerCameraManager->bClientSimulatingViewTarget = false;
}

void ASpearmanPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	LocalTickRate = (1.f / DeltaTime);

	/*TestDeltaTimeSum += DeltaTime;
	if (TestDeltaTimeSum >= 1.f && HasAuthority())
	{
		TestDeltaTimeSum = 0.f;
		if (GEngine)
		{
			FString str = FString::SanitizeFloat(GetWorld()->GetTimeSeconds());
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.f,
				FColor::Yellow,
				*str
			);
		}
	}*/

	if (IsLocalController())
	{
		SetHUDTickRate(LocalTickRate, ServerTickRate);
		SetHUDTime();
		SetHUDPing(DeltaTime);
		SetHUDAlive();

		HUDInit();
	}
}

void ASpearmanPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpearmanPlayerController, MatchState);
}

void ASpearmanPlayerController::OnPossess(APawn* InPawn)
{ /* Server Only */
	Super::OnPossess(InPawn);
	
	Inventory->SetSpearmanCharacter(Cast<ASpearmanCharacter>(InPawn));

	PlayerCameraManager->ViewPitchMin = -45.f;
	PlayerCameraManager->ViewPitchMax = 45.f;

	InitRenderTargetIfServer(InPawn);
}

void ASpearmanPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ASpearmanPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent == nullptr) return;

	InputComponent->BindAction("Quit", IE_Pressed, this, &ASpearmanPlayerController::ShowReturnToMainMenu);
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
			int32 Seconds = CountdownTime - Minutes * 60;

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

void ASpearmanPlayerController::SetHUDCooldownCountdown(float CountdownTime)
{
	SpearmanHUD = (SpearmanHUD == nullptr) ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;

	if (SpearmanHUD && SpearmanHUD->CharacterOverlayCooldown)
	{
		if (SpearmanHUD->CharacterOverlayCooldown->CooldownTimeText)
		{
			if (CountdownTime < 0.f)
			{
				SpearmanHUD->CharacterOverlayCooldown->CooldownTimeText->SetText(FText());
				return;
			}

			int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
			int32 Seconds = CountdownTime - Minutes * 60;

			FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
			SpearmanHUD->CharacterOverlayCooldown->CooldownTimeText->SetText(FText::FromString(CountdownText));
		}
	}
}

void ASpearmanPlayerController::SetHUDBalance(int32 Balance)
{
	SpearmanHUD = (SpearmanHUD == nullptr) ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;

	if (SpearmanHUD && SpearmanHUD->CharacterOverlay && SpearmanHUD->CharacterOverlay->InventoryWidget)
	{
		FString BalanceText = FString::Printf(TEXT("%d"), Balance);
		SpearmanHUD->CharacterOverlay->InventoryWidget->BalanceText->SetText(FText::FromString(BalanceText));
	}
}

void ASpearmanPlayerController::SetHUDBalanceRanking()
{ /* Owning Client Only */
	AGameStateBase* GameState = UGameplayStatics::GetGameState(this);
	if (GameState)
	{
		TArray<TObjectPtr<APlayerState>> PlayerArray = GameState->PlayerArray;

		TArray<ASpearmanPlayerState*> SpearmanPlayerStateArray;
		SpearmanPlayerStateArray.Reserve(PlayerArray.Num());
		for (const TObjectPtr<APlayerState>& PS : PlayerArray)
		{
			ASpearmanPlayerState* SpearmanPlayerState = Cast<ASpearmanPlayerState>(PS);
			UE_LOG(LogTemp, Warning, TEXT("PlayerState->Balance : %d"), SpearmanPlayerState->GetBalance());
			SpearmanPlayerStateArray.Add(SpearmanPlayerState);
		}

		SpearmanPlayerStateArray.Sort([](ASpearmanPlayerState& A, ASpearmanPlayerState& B)
			{
				return (A.GetBalance() > B.GetBalance());
			});

		if (SpearmanHUD && SpearmanHUD->CharacterOverlay)
		{
			FString RichestText = FString::Printf(TEXT("%d"), SpearmanPlayerStateArray[0]->GetBalance());
			SpearmanHUD->CharacterOverlay->Richest_Text->SetText(FText::FromString(RichestText));
		}
	}
}

void ASpearmanPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + BeginPlayTime;
		HandleWaitingToStart();
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

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart)
		{
			SetHUDNoticeCountdown(TimeLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
		else if (MatchState == MatchState::Cooldown)
		{
			SetHUDCooldownCountdown(TimeLeft);
		}
	}
	 
	CountdownInt = SecondsLeft;
}

void ASpearmanPlayerController::HUDInit()
{ /* Owning Client Only, Character may be nullptr in Beginplay, so Initialize HUD in Tick() once */
	if (CharacterOverlay == nullptr)
	{
		if (SpearmanHUD && SpearmanHUD->CharacterOverlay)
		{
			CharacterOverlay = SpearmanHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHp(HUDHp, HUDMaxHp);
				SetHUDBalance(GetPlayerState<ASpearmanPlayerState>()->GetBalance());
				SetHUDBalanceRanking();
				CharacterOverlay->InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
				CharacterOverlay->BlueZoneImage->SetVisibility(ESlateVisibility::Hidden);
				CharacterOverlay->ItemSaleWidget->SetVisibility(ESlateVisibility::Hidden);

				SpearmanCharacter = (SpearmanCharacter == nullptr) ? Cast<ASpearmanCharacter>(GetPawn()) : SpearmanCharacter;
				if (SpearmanCharacter && IsLocalController())
				{
					UMaterialInstance* MinimapMatInst = LoadObject<UMaterialInstance>(nullptr, TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Assets/Textures/Minimap/RenderTarget_Mat_Inst.RenderTarget_Mat_Inst'"));
					if (MinimapMatInst)
					{
						UMaterialInstanceDynamic* MiniMapMatInstDynamic = UMaterialInstanceDynamic::Create(MinimapMatInst, this);
						if (MiniMapMatInstDynamic && SpearmanCharacter->RenderTargetMinimap)
						{
							MiniMapMatInstDynamic->SetTextureParameterValue(FName("MinimapParam"), SpearmanCharacter->RenderTargetMinimap);
							CharacterOverlay->Minimap->SetBrushFromMaterial(MiniMapMatInstDynamic);
						}
					}
				}
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

void ASpearmanPlayerController::SetHUDAlive()
{ // TODO : Bebug
	SpearmanHUD = (SpearmanHUD == nullptr) ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;
	if (SpearmanHUD && CharacterOverlay)
	{
		FString AliveText = FString::Printf(TEXT("%d"), AliveCount);
		CharacterOverlay->Alive_Text->SetText(FText::FromString(AliveText));
	}
}

void ASpearmanPlayerController::InitRenderTargetIfServer(APawn* InPawn)
{
	SpearmanCharacter = (SpearmanCharacter == nullptr) ? Cast<ASpearmanCharacter>(InPawn) : SpearmanCharacter;
	if (SpearmanCharacter && IsLocalController())
	{
		SetHUDHp(SpearmanCharacter->GetHp(), SpearmanCharacter->GetMaxHp());

		UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(this);
		if (RenderTarget)
		{
			RenderTarget->InitAutoFormat(1024, 1024);
			RenderTarget->UpdateResource();
			SpearmanCharacter->RenderTargetMinimap = RenderTarget;
		}
		if (SpearmanCharacter->RenderTargetMinimap)
		{
			SpearmanCharacter->MinimapSceneCapture->TextureTarget = SpearmanCharacter->RenderTargetMinimap;
		}
	}
}

void ASpearmanPlayerController::RequestServerTime()
{
	ServerRequestServerTime(GetWorld()->GetTimeSeconds());
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

void ASpearmanPlayerController::HandleWaitingToStart()
{
	if (SpearmanHUD && SpearmanHUD->CharacterOverlayCooldown)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Yellow,
				FString(TEXT("Cooldown RemoveFromParent"))
			);
		}
		SpearmanHUD->CharacterOverlayCooldown->RemoveFromParent();

		CharacterOverlay = nullptr;
		SpearmanHUD = nullptr;
		SpearmanCharacter = nullptr;
		SpearmanGameMode = nullptr;
	}
}

void ASpearmanPlayerController::HandleMatchHasStarted()
{
	SpearmanHUD = (SpearmanHUD == nullptr) ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;
	if (SpearmanHUD)
	{
		SpearmanHUD->AddCharacterOverlay();

		FInputModeGameOnly InputModeData;
		SetInputMode(InputModeData);
		SetShowMouseCursor(false);

		if (SpearmanHUD->CharacterOverlayNotice)
		{
			SpearmanHUD->CharacterOverlayNotice->RemoveFromParent();
		}
	}
}

void ASpearmanPlayerController::HandleCooldown()
{
	SpearmanHUD = (SpearmanHUD == nullptr) ? Cast<ASpearmanHUD>(GetHUD()) : SpearmanHUD;
	if (SpearmanHUD)
	{
		SpearmanHUD->CharacterOverlay->RemoveFromParent();
		SpearmanHUD->AddCharacterOverlayCooldown();

		if (SpearmanHUD->CharacterOverlayCooldown)
		{		
			FString BalanceText = FString::Printf(TEXT("%d"), GetPlayerState<ASpearmanPlayerState>()->GetBalance());
			SpearmanHUD->CharacterOverlay->InventoryWidget->BalanceText->SetText(FText::FromString(BalanceText));

			FString NoticeText("Match end ! New match soon !");
			SpearmanHUD->CharacterOverlayCooldown->CooldownNoticeText->SetText(FText::FromString(NoticeText));
		}
	}
	SpearmanCharacter = (SpearmanCharacter == nullptr) ? Cast<ASpearmanCharacter>(GetPawn()) : SpearmanCharacter;
	if (SpearmanCharacter)
	{
		SpearmanCharacter->bDisableKeyInput = true;
	}
}

void ASpearmanPlayerController::HandleExtraction()
{ /* Server Only */
	ClientEnableItemSale();

	FTimerHandle ExtractionTimer;
	GetWorldTimerManager().SetTimer(ExtractionTimer, this, &ASpearmanPlayerController::ExtractionCallback, 10.f);
}

void ASpearmanPlayerController::ExtractionCallback()
{
	ClientSetSpectatorHUD();

	SetPlayerSpectate();
}

void ASpearmanPlayerController::ClientEnableItemSale_Implementation()
{ // TOOD : UI로만 판매유무를 확인하면 안되고, 서버측에서 탈출했는지 유무를 확인할 방법이 있어야 함.
	ShowInventoryWidget();
	SpearmanHUD->CharacterOverlay->ItemSaleWidget->SetVisibility(ESlateVisibility::Visible);
	SpearmanHUD->CharacterOverlay->ExtractionNoticeText->SetVisibility(ESlateVisibility::Visible);
}

void ASpearmanPlayerController::ClientSetSpectatorHUD_Implementation()
{
	if (SpearmanHUD->CharacterOverlay)
	{
		SpearmanHUD->CharacterOverlay->ItemSaleWidget->SetVisibility(ESlateVisibility::Collapsed);
		SpearmanHUD->CharacterOverlay->ExtractionNoticeText->SetVisibility(ESlateVisibility::Collapsed);
		SpearmanHUD->CharacterOverlay->HpBar->SetVisibility(ESlateVisibility::Collapsed);
		SpearmanHUD->CharacterOverlay->HpText->SetVisibility(ESlateVisibility::Collapsed);
		SpearmanHUD->CharacterOverlay->InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
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
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(InventoryWidget->TakeWidget());
			SetInputMode(InputModeData);
			SetShowMouseCursor(true);
		}
	}
}

void ASpearmanPlayerController::SetPlayerPlay()
{ /* Server Only */
	if (!HasAuthority()) return;

	// Update the state on server
	PlayerState->SetIsSpectator(false);
	ChangeState(NAME_Playing);

	bPlayerIsWaiting = false;

	// Push the state update to the client
	ClientGotoState(NAME_Playing);

	// Update the HUD to remove the spectator screen
	ClientHUDStateChanged(EHUDState::EHS_Playing);
}

void ASpearmanPlayerController::SetPlayerSpectate()
{ /* Server Only */
	if (!HasAuthority()) return;

	// Update the state on server
	PlayerState->SetIsSpectator(true);
	ChangeState(NAME_Spectating);

	bPlayerIsWaiting = true;

	// Push the state update to the client
	ClientGotoState(NAME_Spectating);

	// View some other alive player (for server-side only)
	ViewAPlayer(+1);

	// Update the HUD to add teh spectator screen
	ClientHUDStateChanged(EHUDState::EHS_Spectating);
}

void ASpearmanPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	if (IsInState(NAME_Spectating))
	{
		ServerViewNextPlayer();
	}
}

void ASpearmanPlayerController::ClientHUDStateChanged_Implementation(EHUDState NewState)
{ /* ClientRPC, Reliable */
	SpearmanHUD = (SpearmanHUD == nullptr) ? GetHUD<ASpearmanHUD>() : SpearmanHUD;
	if (SpearmanHUD)
	{
		SpearmanHUD->OnHUDStateChanged(NewState);
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
{ // Server Only, Client should get MatchState from Server ASAP. 
	SpearmanGameMode = (SpearmanGameMode == nullptr) ? Cast<ASpearmanGameMode>(UGameplayStatics::GetGameMode(this)) : SpearmanGameMode;
	if (SpearmanGameMode)
	{
		MatchState = SpearmanGameMode->GetMatchState();
		BeginPlayTime = SpearmanGameMode->BeginPlayTime;
		WarmupTime = SpearmanGameMode->WarmupTime;
		MatchTime = SpearmanGameMode->MatchTime;
		CooldownTime = SpearmanGameMode->CooldownTime;
		ClientReportMatchState(MatchState, BeginPlayTime, WarmupTime, MatchTime, CooldownTime);
	}
}

void ASpearmanPlayerController::ClientReportMatchState_Implementation(FName ServerMatchState, float ServerBeginPlayTime, float ServerWarmupTime, float ServerMatchTime, float ServerCooldownTime)
{ /* Client Only */
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

void ASpearmanPlayerController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenuWidget == nullptr) return;
	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}
	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}

}