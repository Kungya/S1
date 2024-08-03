// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemSpawnerComponent.h"
#include "Spearman/Items/Item.h"
#include "Spearman/GameMode/SpearmanGameMode.h"

UItemSpawnerComponent::UItemSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
}

void UItemSpawnerComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UItemSpawnerComponent::SpawnItem()
{ /* Server Only */
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority()) return;
	if (GetWorld()->GetAuthGameMode<ASpearmanGameMode>()->GetMatchState() != MatchState::InProgress) return;

	const int32 RandNumForItemId = FMath::RandRange(1, 5);

	AItem* ItemToSpawn = GetWorld()->SpawnActorDeferred<AItem>(ItemClass, GetOwner()->GetActorTransform());
	if (ItemToSpawn)
	{
		ItemToSpawn->Init(RandNumForItemId);
		ItemToSpawn->FinishSpawning(GetOwner()->GetActorTransform());
	}
}

void UItemSpawnerComponent::SpawnItemTimer()
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority()) return;
	
	FTimerHandle ItemSpawnerHandle;
	GetWorld()->GetTimerManager().SetTimer(ItemSpawnerHandle, this, &UItemSpawnerComponent::SpawnItem, 3.f, true, 3.f);
}
