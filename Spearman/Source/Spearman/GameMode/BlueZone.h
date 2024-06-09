// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlueZone.generated.h"

USTRUCT(BlueprintType)
struct FBlueZoneInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "BlueZoneInfo")
	float WaitingTime;

	UPROPERTY(EditAnywhere, Category = "BlueZoneInfo")
	float MovingTime;

	UPROPERTY(EditAnywhere, Category = "BlueZoneInfo")
	float ScaleToDecrease;
};

UCLASS()
class SPEARMAN_API ABlueZone : public AActor
{
	GENERATED_BODY()

public:	
	ABlueZone();

	void StartMovingBlueZone();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
private:
	
	UPROPERTY(EditAnywhere, Category = "Components")
	UStaticMeshComponent* ZoneMesh;
	
	// Bluezone Info per Phase 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueZoneInfo", meta = (AllowPrivateAccess = "true"))
	TArray<FBlueZoneInfo> BlueZoneInfoArray;
	
	UPROPERTY()
	int32 CurrentPhase;

	FTimerHandle MovingTimerHandle;

	void ReduceBlueZone();
	void StopBlueZone();

public:


};
