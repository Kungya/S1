// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanPlayerState.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

void ASpearmanPlayerState::BeginPlay()
{
	Super::BeginPlay();

	//if (HasAuthority())
	//{
	//	// TODO : Better TeamNumber selection algorithm.
	//	if (FMath::RandBool())
	//	{
	//		SetTeam(0);
	//	}
	//	else
	//	{
	//		SetTeam(1);
	//	}

	//	if (const UNetDriver* NetDriver = GetWorld()->GetNetDriver())
	//	{
	//		if (US1ReplicationGraph* ReplicationGraph = NetDriver->GetReplicationDriver<US1ReplicationGraph>())
	//		{
	//			ReplicationGraph->SetTeamForPlayerController(GetPlayerController(), Team);
	//		}
	//	}
	//}
}

void ASpearmanPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ASpearmanPlayerState, Balance, SharedParams);
	//DOREPLIFETIME_WITH_PARAMS_FAST(ASpearmanPlayerState, Team, SharedParams);
}

void ASpearmanPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	// TODO : Set New Team After SeamlessTravel if you want
	ASpearmanPlayerState* SpearmanPlayerState = Cast<ASpearmanPlayerState>(PlayerState);
	if (SpearmanPlayerState)
	{
		SpearmanPlayerState->SetBalance(Balance);
	}
}

//void ASpearmanPlayerState::SetTeam(int32 NewTeam)
//{
//	MARK_PROPERTY_DIRTY_FROM_NAME(ASpearmanPlayerState, Team, this);
//	Team = NewTeam;
//}
//
//void ASpearmanPlayerState::OnRep_Team()
//{
//	if (HasAuthority())
//	{
//		UE_LOG(LogTemp, Warning, TEXT("ServerPlayer %s's TeamId : %d"), *GetPlayerName(), Team);
//	}
//	else
//	{
//		UE_LOG(LogTemp, Warning, TEXT("Player %s's TeamId : %d"), *GetPlayerName(), Team);
//	}
//}

void ASpearmanPlayerState::SetBalance(int32 NewBalance)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ASpearmanPlayerState, Balance, this);
	Balance = NewBalance;

	OnRep_Balance();
}

void ASpearmanPlayerState::OnRep_Balance()
{
	SpearmanPlayerController = (SpearmanPlayerController == nullptr) ? Cast<ASpearmanPlayerController>(GetPlayerController()) : SpearmanPlayerController;
	if (SpearmanPlayerController)
	{
		SpearmanPlayerController->SetHUDBalance(Balance);
	}
}