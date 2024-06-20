// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class ASpearmanCharacter;
class ASpearmanPlayerController;
class AWeapon;

USTRUCT(BlueprintType)
struct FHitBox
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector Extent;
};

USTRUCT(BlueprintType)
struct FSavedFrame
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TArray<FHitBox> SavedHitBoxArray;
};

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

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void ShowSavedFrame(const FSavedFrame& Frame, const FColor& Color);

	UFUNCTION(Server, Reliable)
	void ServerRewindRequest(ASpearmanCharacter* HitSpearmanCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, AWeapon* Weapon);

protected:
	virtual void BeginPlay() override;

	void SaveFrame(FSavedFrame& OutFrame);
	
	FSavedFrame GetInterpFrame(const FSavedFrame& Next, const FSavedFrame& Prev, float HitTime);
	FRewindResult Rewind(ASpearmanCharacter* HitSpearmanCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTimem, AWeapon* Weapon);
	FRewindResult SimulateHit(ASpearmanCharacter* HitSpearmanCharacter, const FVector_NetQuantize& TraceStart, const FSavedFrame& Frame, const FVector_NetQuantize& HitLocation, AWeapon* Weapon);

	void ReserveCurrentFrame(ASpearmanCharacter* HitSpearmanCharacter, FSavedFrame& OUT Frame);
	void MoveHitBoxes(ASpearmanCharacter* HitSpearmanCharacter, const FSavedFrame& Frame);
	void ResetHitBoxes(ASpearmanCharacter* HitSpearmanCharacter, const FSavedFrame& Frame);
	void SaveCurrentFrame();

private:
	UPROPERTY()
	ASpearmanCharacter* SpearmanCharacter;

	UPROPERTY()
	ASpearmanPlayerController* SpearmanPlayerController;

	/*
	* Recorded Frame per Tick
	*/

	TDoubleLinkedList<FSavedFrame> HistoricalBuffer;
	

	// LimitTime, 1.f == 1000ms
	UPROPERTY(EditAnywhere)
	float RewindLimitTime = 3.f;

public:

		
};
