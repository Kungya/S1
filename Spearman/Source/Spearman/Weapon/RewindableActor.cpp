// Fill out your copyright notice in the Description page of Project Settings.

#include "RewindableActor.h"
#include "Components/BoxComponent.h"
#include "Spearman/SpearComponents/HistoryComponent.h"

ARewindableActor::ARewindableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	/* Server Only Component */
	History = CreateDefaultSubobject<UHistoryComponent>(TEXT("HistoryComponent"));
}

void ARewindableActor::BeginPlay()
{
	Super::BeginPlay();
}

TArray<UBoxComponent*>& ARewindableActor::GetHitBoxArray()
{
	return HitBoxArray;
}
