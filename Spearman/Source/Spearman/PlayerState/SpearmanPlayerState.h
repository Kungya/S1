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
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;

	/*UFUNCTION()
	void OnRep_Team();*/

	UFUNCTION()
	void OnRep_Balance();

private:
	UPROPERTY()
	ASpearmanPlayerController* SpearmanPlayerController;
	
	/*UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Team)
	int32 Team = -1;*/

	UPROPERTY(ReplicatedUsing = OnRep_Balance)
	int32 Balance = 0;



public:
	/*void SetTeam(int32 NewTeam);
	FORCEINLINE int32 GetTeam() const { return Team; }*/
	void SetBalance(int32 NewBalance);
	FORCEINLINE int32 GetBalance() const { return Balance; }

};
