// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSpawnerActor.generated.h"

class UItemSpawnerComponent;



UCLASS()
class SPEARMAN_API AItemSpawnerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AItemSpawnerActor();

protected:
	virtual void BeginPlay() override;


private:
	UPROPERTY(EditAnywhere, Category="StaticMesh")
	UStaticMeshComponent* Mesh;
	
	UPROPERTY(VisibleAnywhere, Category="ItemSpawner")
	UItemSpawnerComponent* ItemSpawner;


public:	

};
