// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanPlayerState.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

void ASpearmanPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ASpearmanPlayerState, Balance, SharedParams);
}

void ASpearmanPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	ASpearmanPlayerState* SpearmanPlayerState = Cast<ASpearmanPlayerState>(PlayerState);
	if (SpearmanPlayerState)
	{
		SpearmanPlayerState->SetBalance(Balance);
	}
}

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