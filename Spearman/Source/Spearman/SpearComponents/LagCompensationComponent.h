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

/*
** HeadShot	: { true, true }
** BodyShot	: { true, false }
** MIss		: { false, false }
*/
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
	void ServerRewindRequest(ARewindableCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* Weapon);

	UFUNCTION(Server, Reliable)
	void ServerRewindRequestForParrying(ARewindableActor* HitWeapon, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* Weapon);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	FSavedFrame GetInterpFrame(const FSavedFrame& Next, const FSavedFrame& Prev, float HitTime);
	FRewindResult Rewind(ARewindableCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* Weapon);
	FRewindResult SimulateHit(ARewindableCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FSavedFrame& Frame, const FVector_NetQuantize& HitLocation, AWeapon* Weapon);
	
	void ReserveCurrentFrame(ARewindableCharacter* HitSpearmanCharacter, FSavedFrame& OutReservedFrame);
	void MoveHitBoxes(ARewindableCharacter* HitSpearmanCharacter, const FSavedFrame& FrameToMove);
	void ResetHitBoxes(ARewindableCharacter* HitSpearmanCharacter, const FSavedFrame& ReservedFrame);

	/* Rewind for Parrying, TODO : integrate common code */
	bool Rewind(ARewindableActor* HitWeapon, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* Weapon);
	bool SimulateHit(ARewindableActor* HitWeapon, const FVector_NetQuantize& TraceStart, const FSavedFrame& Frame, const FVector_NetQuantize& HitLocation, AWeapon* Weapon);

	void ReserveCurrentFrame(AActor* HitActor, FSavedFrame& OutReservedFrame);
	void MoveHitBoxes(AActor* HitActor, const FSavedFrame& Frame);
	void ResetHitBoxes(AActor* HitActor, const FSavedFrame& ReservedFrame);

private:
	// Caching Attacker SpearmanCharacter
	UPROPERTY()
	ASpearmanCharacter* SpearmanCharacter;

public:

		
};
