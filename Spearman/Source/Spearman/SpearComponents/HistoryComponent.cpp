// Fill out your copyright notice in the Description page of Project Settings.


#include "HistoryComponent.h"
#include "Components/BoxComponent.h"
#include "Spearman/Character/RewindableCharacter.h"

UHistoryComponent::UHistoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;


}

void UHistoryComponent::BeginPlay()
{
	Super::BeginPlay();

	RewindableCharacter = (RewindableCharacter == nullptr) ? Cast<ARewindableCharacter>(GetOwner()) : RewindableCharacter;

	// UE_LOG(LogTemp, Warning, TEXT("Rewindable HitBoxArray Num() : %d"), RewindableCharacter->HitBoxArray.Num());
}

void UHistoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveCurrentFrame();
}

void UHistoryComponent::SaveCurrentFrame()
{ /* Server Only */
	RewindableCharacter = (RewindableCharacter == nullptr) ? Cast<ARewindableCharacter>(GetOwner()) : RewindableCharacter;
	if (RewindableCharacter == nullptr) return;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		/** keep Time length of HistroicalBuffer at RewindLimitTime  */
		/* (Old) [Tail] - [Frame] - [Frame] - [...] - [Frame] - [Frame] - [Head] (Recent) */
		if (HistoricalBuffer.Num() >= 2)
		{
			float BufferTimeLength = (HistoricalBuffer.GetHead()->GetValue().Time - HistoricalBuffer.GetTail()->GetValue().Time);
			while (BufferTimeLength > RewindLimitTime)
			{
				HistoricalBuffer.RemoveNode(HistoricalBuffer.GetTail());
				BufferTimeLength = (HistoricalBuffer.GetHead()->GetValue().Time - HistoricalBuffer.GetTail()->GetValue().Time);
			}
		}

		/* Save Current Frame's HitBox Information */
		FSavedFrame CurrentFrame;
		CurrentFrame.Time = GetWorld()->GetTimeSeconds();

		for (UBoxComponent* Box : RewindableCharacter->HitBoxArray)
		{
			if (Box)
			{
				FHitBox HitBox;
				HitBox.Location = Box->GetComponentLocation();
				HitBox.Rotation = Box->GetComponentRotation();
				HitBox.Extent = Box->GetScaledBoxExtent();
				CurrentFrame.SavedHitBoxArray.Add(HitBox);
			}
		}
		
		HistoricalBuffer.AddHead(CurrentFrame);
	}
}