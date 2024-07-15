// Fill out your copyright notice in the Description page of Project Settings.

#include "RewindableCharacter.h"
#include "Components/BoxComponent.h"
#include "Spearman/SpearComponents/HistoryComponent.h"

ARewindableCharacter::ARewindableCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	/* Server Only Component */
	History = CreateDefaultSubobject<UHistoryComponent>(TEXT("HistoryComponent"));
}

void ARewindableCharacter::BeginPlay()
{
	Super::BeginPlay();
}