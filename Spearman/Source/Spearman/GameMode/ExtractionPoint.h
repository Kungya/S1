// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExtractionPoint.generated.h"

class USphereComponent;
class ASpearmanCharacter;


UCLASS()
class SPEARMAN_API AExtractionPoint : public AActor
{
	GENERATED_BODY()
	
public:
	AExtractionPoint();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(
			UPrimitiveComponent* OverlappedComponent,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex,
			bool bFromSweep,
			const FHitResult& SweepResult
		);

	UFUNCTION()
	virtual void OnSphereEndOverlap(
			UPrimitiveComponent* OverlappedComponent,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex
		);

	void Extraction();

private:
	
	UPROPERTY(VisibleAnywhere, Category = "ExtractionPoint Properties")
	USceneComponent* SceneComponent;
	
	UPROPERTY(EditAnywhere, Category = "ExtractionPoint Properties")
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(EditAnywhere, Category = "ExtractionPoint Properties")
	USphereComponent* OverlapSphere;

	TWeakObjectPtr<ASpearmanCharacter> CharacterToExtract;

	FTimerHandle ExtractionTimerHandle;

};