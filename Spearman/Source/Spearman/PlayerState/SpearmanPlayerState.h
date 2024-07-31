// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SpearmanPlayerState.generated.h"

class ASpearmanPlayerController;

UCLASS()
class SPEARMAN_API ASpearmanPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

protected:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
	
	virtual void CopyProperties(APlayerState* PlayerState) override;

	UFUNCTION()
	void OnRep_Balance();

private:
	UPROPERTY()
	ASpearmanPlayerController* SpearmanPlayerController;
	
	UPROPERTY(ReplicatedUsing = OnRep_Balance)
	int32 Balance = 0;



public:
	void SetBalance(int32 NewBalance);
	FORCEINLINE int32 GetBalance() const { return Balance; }

};
