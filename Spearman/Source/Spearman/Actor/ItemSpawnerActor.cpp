// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemSpawnerActor.h"
#include "Spearman/SpearComponents/ItemSpawnerComponent.h"

AItemSpawnerActor::AItemSpawnerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	
	ItemSpawner = CreateDefaultSubobject<UItemSpawnerComponent>(TEXT("ItemSpawnerComponent"));
}

void AItemSpawnerActor::BeginPlay()
{
	Super::BeginPlay();

	ItemSpawner->SpawnItemTimer();
}

