// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
//#include "Spearman/Character/RewindableCharacter.h"
#include "LagCompensationComponent.generated.h"

class ASpearmanCharacter;
class ABasicMonster;
class AWeapon;
class ARewindableCharacter;
class AHistoryCoomponent;
struct FSavedFrame;

/* {false, false}, {true, false}, {true, true} */
USTRUCT(BlueprintType)
struct FRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHit;

	UPROPERTY()
	bool bHeadShot;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPEARMAN_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();
	friend class ASpearmanCharacter;
	
	void ShowSavedFrame(const FSavedFrame& Frame, const FColor& Color);

	UFUNCTION(Server, Reliable)
	void ServerRewindRequest(ARewindableCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, AWeapon* Weapon);

protected:
	virtual void BeginPlay() override;
	
	FSavedFrame GetInterpFrame(const FSavedFrame& Next, const FSavedFrame& Prev, float HitTime);
	FRewindResult Rewind(ARewindableCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTimem, AWeapon* Weapon);
	FRewindResult SimulateHit(ARewindableCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FSavedFrame& Frame, const FVector_NetQuantize& HitLocation, AWeapon* Weapon);

	void ReserveCurrentFrame(ARewindableCharacter* HitSpearmanCharacter, FSavedFrame& OutFrame);
	void MoveHitBoxes(ARewindableCharacter* HitSpearmanCharacter, const FSavedFrame& Frame);
	void ResetHitBoxes(ARewindableCharacter* HitSpearmanCharacter, const FSavedFrame& Frame);

private:
	// Caching Attacker SpearmanCharacter
	UPROPERTY()
	ASpearmanCharacter* SpearmanCharacter;

public:

		
};
