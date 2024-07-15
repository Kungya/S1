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

}

void UHistoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RecordCurrentFrame();
}

void UHistoryComponent::RecordCurrentFrame()
{ /* Server Only */
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

		RewindableInterface = (RewindableInterface == nullptr) ? Cast<IRewindableInterface>(GetOwner()) : RewindableInterface;
		if (RewindableInterface)
		{
			const TArray<UBoxComponent*>& HitBoxArray = RewindableInterface->GetHitBoxArray();
			for (UBoxComponent* Box : HitBoxArray)
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
}