// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class ASpearmanCharacter;
class ABasicMonster;
class AWeapon;
class ARewindableCharacter;
class AHistoryCoomponent;
class IRewindableInterface;
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
	void ServerRewindRequest(ARewindableCharacter* HitRewindableCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* Weapon);

	UFUNCTION(Server, Reliable)
	void ServerRewindRequestForParrying(ARewindableActor* HitRewindableActor, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* Weapon);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	FRewindResult Rewind(ARewindableCharacter* HitRewindableCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* Weapon);
	FRewindResult SimulateHit(ARewindableCharacter* HitRewindableCharacter, const FVector_NetQuantize& TraceStart, const FSavedFrame& Frame, const FVector_NetQuantize& HitLocation, AWeapon* Weapon);

	bool Rewind(ARewindableActor* HitRewindableActor, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* Weapon);
	bool SimulateHit(ARewindableActor* HitRewindableActor, const FVector_NetQuantize& TraceStart, const FSavedFrame& Frame, const FVector_NetQuantize& HitLocation, AWeapon* Weapon);

	FSavedFrame GetInterpFrame(const FSavedFrame& Next, const FSavedFrame& Prev, float HitTime);

	void ReserveCurrentFrame(IRewindableInterface* RewindableInterface, FSavedFrame& OutReservedFrame);
	void MoveHitBoxes(IRewindableInterface* RewindableInterface, const FSavedFrame& Frame);
	void ResetHitBoxes(IRewindableInterface* RewindableInterface, const FSavedFrame& ReservedFrame);

private:
	// Caching Attacker SpearmanCharacter
	UPROPERTY()
	ASpearmanCharacter* SpearmanCharacter;

public:

		
};
