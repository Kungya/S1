// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Spearman/Interfaces/RewindableInterface.h"
#include "HistoryComponent.generated.h"

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

	/** Record Frame per Tick */
	TDoubleLinkedList<FSavedFrame> HistoricalBuffer;

protected:
	virtual void BeginPlay() override;

	void RecordCurrentFrame();

private:

	/** LimitTime, 1.f == 1'000ms */ 
	UPROPERTY(EditAnywhere)
	float RewindLimitTime = 1.f;

public:

};