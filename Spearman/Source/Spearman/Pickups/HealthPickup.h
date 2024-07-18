// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "HealthPickup.generated.h"

/**
 * 
 */
UCLASS()
class SPEARMAN_API AHealthPickup : public APickup
{
	GENERATED_BODY()
public:
	AHealthPickup();

protected:
	virtual void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent
		, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, 
		bool bFromSweep, 
		const FHitResult& SweepResult
	) override;

private:
	// Heal (HealAmount / HealingTime) per Sec, for HealingTIme.
	UPROPERTY(EditAnywhere)
	float HealAmount = 50;

	UPROPERTY(EditAnywhere)
	float HealingTime = 10;
public:
	
};
