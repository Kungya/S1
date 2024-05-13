// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlueZone.generated.h"

class UCapsuleComponent;

UCLASS()
class SPEARMAN_API ABlueZone : public AActor
{
	GENERATED_BODY()
	
public:	
	ABlueZone();
	
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

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
	virtual void OnSafeZoneBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	virtual void OnSafeZoneEndOverlap(
			UPrimitiveComponent* OverlappedComponent,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex
		);
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Component")
	USceneComponent* Scene;
	
	UPROPERTY(EditAnywhere, Category = "Component")
	UCapsuleComponent* SafeZone;
	
	UPROPERTY(EditAnywhere, Category = "Component")
	UStaticMeshComponent* SafeZoneMesh;

	UPROPERTY(EditAnywhere, Category = "Component")
	UCapsuleComponent* BlueZone;


	

	float DefaultCapsuleHalfHeight = 2000.f;
	float DefaultCapsuleRadius = 1500.f;

public:


};
