// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HistoryComponent.generated.h"

class ARewindableCharacter;
class UBoxComponent;

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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPEARMAN_API UHistoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHistoryComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/*
	* Recorded Frame per Tick
	*/

	TDoubleLinkedList<FSavedFrame> HistoricalBuffer;

protected:
	virtual void BeginPlay() override;

	void SaveCurrentFrame();
	void SaveFrame(FSavedFrame& OutFrame);

private:
	UPROPERTY()
	ARewindableCharacter* RewindableCharacter;

	// LimitTime, 1.f == 1000ms
	UPROPERTY(EditAnywhere)
	float RewindLimitTime = 1.f;

public:

			
};