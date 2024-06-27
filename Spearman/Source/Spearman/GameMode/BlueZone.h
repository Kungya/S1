// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlueZone.generated.h"

USTRUCT(BlueprintType)
struct FBlueZoneInfo
{
	GENERATED_BODY()

	UPROPERTY()
	float WaitingTime;

	UPROPERTY()
	float MovingTime;

	UPROPERTY()
	float ScaleToDecreasePerLoop;
};

UCLASS()
class SPEARMAN_API ABlueZone : public AActor
{
	GENERATED_BODY()

public:	
	ABlueZone();

	void StartMovingBlueZone();
	void ReduceBlueZone();
	void StopBlueZone();

	void CalcMoveVector();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION()
	virtual void OnBlueZoneBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	virtual void OnBlueZoneEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

private:
	
	UPROPERTY(EditAnywhere, Category = "Components")
	UStaticMeshComponent* ZoneMesh;
	
	// Bluezone Info per Phase 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueZoneInfo", meta = (AllowPrivateAccess = "true"))
	TArray<FBlueZoneInfo> BlueZoneInfoArray;
	
	UPROPERTY(EditAnywhere, Category = "BlueZoneInfo")
	int32 CurrentPhase = 0;

	UPROPERTY(EditAnyWhere, Category = "BlueZoneInfo")
	float TimerInterval = 0.2f;

	FVector NormalizedRandomVector = FVector::ZeroVector;

	FTimerHandle MovingTimerHandle;

public:


};
