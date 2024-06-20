// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Spearman/SpearComponents/CombatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Spearman/Weapon/Weapon.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveCurrentFrame();
}

void ULagCompensationComponent::SaveCurrentFrame()
{ /* Server Only */
	if (SpearmanCharacter && SpearmanCharacter->HasAuthority())
	{
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

		FSavedFrame CurrentFrame;
		SaveFrame(CurrentFrame);
		HistoricalBuffer.AddHead(CurrentFrame);
	}
}

void ULagCompensationComponent::SaveFrame(FSavedFrame& OUT Frame)
{ /* Server Only, Save Current Frame's HitBox Information */
	SpearmanCharacter = (SpearmanCharacter == nullptr) ? Cast<ASpearmanCharacter>(GetOwner()) : SpearmanCharacter; 
	if (SpearmanCharacter)
	{
		Frame.Time = GetWorld()->GetTimeSeconds();

		for (UBoxComponent* Box : SpearmanCharacter->HitBoxArray)
		{
			FHitBox HitBox;
			HitBox.Location = Box->GetComponentLocation();
			HitBox.Rotation = Box->GetComponentRotation();
			HitBox.Extent = Box->GetScaledBoxExtent();
			Frame.SavedHitBoxArray.Add(HitBox);
		}
	}
}

void ULagCompensationComponent::ServerRewindRequest_Implementation(ASpearmanCharacter* HitSpearmanCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, AWeapon* Weapon)
{
	if (HitSpearmanCharacter == nullptr || Weapon == nullptr) return;

	FRewindResult RewindResult = Rewind(HitSpearmanCharacter, TraceStart, HitLocation, HitTime, Weapon);
	if (RewindResult.bHit)
	{
		bool bHeadShot = false;
		float HitPartDamage = Weapon->GetDamage();

		if (RewindResult.bHeadShot)
		{
			bHeadShot = true;
			HitPartDamage = Weapon->GetHeadShotDamage();
		}

		const float Dist = FVector::Distance(SpearmanCharacter->GetActorLocation(), HitSpearmanCharacter->GetActorLocation());

		FVector2D InRange(60.f, 240.f);
		FVector2D OutRange(HitPartDamage / 3.f, HitPartDamage);
		const float InDamage = FMath::GetMappedRangeValueClamped(InRange, OutRange, Dist);

		UGameplayStatics::ApplyDamage(HitSpearmanCharacter, InDamage, SpearmanCharacter->Controller, Weapon, UDamageType::StaticClass());
		/* Execute visual effect when hit, Unreliable */
		Weapon->MulticastHit(HitSpearmanCharacter, FMath::CeilToInt(InDamage), HitLocation, bHeadShot);
		UE_LOG(LogTemp, Warning, TEXT("Rewind Success"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Rewind Success, but Miss in Server"));
	}
}

FRewindResult ULagCompensationComponent::Rewind(ASpearmanCharacter* HitSpearmanCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* Weapon)
{
	if (HitSpearmanCharacter == nullptr || HitSpearmanCharacter->GetLagCompensation() == nullptr) return FRewindResult();
	if (HitSpearmanCharacter->GetLagCompensation()->HistoricalBuffer.GetHead() == nullptr || HitSpearmanCharacter->GetLagCompensation()->HistoricalBuffer.GetTail() == nullptr) return FRewindResult();

	// Historical Buffer of HitSpearmanCharacter, Not Attacker
	const TDoubleLinkedList<FSavedFrame>& Buffer = HitSpearmanCharacter->GetLagCompensation()->HistoricalBuffer;
	const float OldestTime = Buffer.GetTail()->GetValue().Time;
	const float NewestTime = Buffer.GetHead()->GetValue().Time;

	FSavedFrame FrameToSimulate;

	/* Can't rewind, Too High Ping */
	if (OldestTime >= HitTime) return FRewindResult();

	/* Don't have to Interp */
	if (NewestTime <= HitTime)
	{
		FrameToSimulate = Buffer.GetHead()->GetValue();
		return SimulateHit(HitSpearmanCharacter, TraceStart, FrameToSimulate, HitLocation, Weapon);
	}

	TDoubleLinkedList<FSavedFrame>::TDoubleLinkedListNode* NextNodeOfHitTime = Buffer.GetHead();
	TDoubleLinkedList<FSavedFrame>::TDoubleLinkedListNode* PrevNodeOfHitTime = Buffer.GetHead();

	/*                   [                 ] - [       ] - [                 ]                      */
	/* (Old) - [Frame] - [NextNodeOfHitTime] - [HitTime] - [PrevNodeOfHitTime] - [Frame] - (Recent) */

	/* Search until Next of HitTime	*/
	while (NextNodeOfHitTime->GetValue().Time > HitTime)
	{
		if (NextNodeOfHitTime->GetNextNode() == nullptr) break;

		NextNodeOfHitTime = NextNodeOfHitTime->GetNextNode();

		if (NextNodeOfHitTime->GetValue().Time > HitTime)
		{
			PrevNodeOfHitTime = NextNodeOfHitTime;
		}
	}
	
	FrameToSimulate = GetInterpFrame(NextNodeOfHitTime->GetValue(), PrevNodeOfHitTime->GetValue(), HitTime);

	return SimulateHit(HitSpearmanCharacter, TraceStart, FrameToSimulate, HitLocation, Weapon);
}

FSavedFrame ULagCompensationComponent::GetInterpFrame(const FSavedFrame& Next, const FSavedFrame& Prev, float HitTime)
{ /* Get Similar HitBox by Interpolating between Next and Prev */
	const float Dist = (Prev.Time - Next.Time);
	const float InterpRatio = FMath::Clamp(((HitTime - Next.Time) / Dist) , 0.f, 1.f);

	FSavedFrame InterpFrame;
	InterpFrame.Time = HitTime;

	for (int32 idx = 0; idx < Prev.SavedHitBoxArray.Num(); idx++)
	{	
		const FHitBox& NextBox = Next.SavedHitBoxArray[idx];
		const FHitBox& PrevBox = Prev.SavedHitBoxArray[idx];

		FHitBox InterpHitBox;
		InterpHitBox.Location = FMath::VInterpTo(NextBox.Location, PrevBox.Location, 1.f, InterpRatio);
		InterpHitBox.Rotation = FMath::RInterpTo(NextBox.Rotation, PrevBox.Rotation, 1.f, InterpRatio);
		InterpHitBox.Extent = PrevBox.Extent;

		InterpFrame.SavedHitBoxArray.Add(InterpHitBox);
	}

	return InterpFrame;
}

FRewindResult ULagCompensationComponent::SimulateHit(ASpearmanCharacter* HitSpearmanCharacter, const FVector_NetQuantize& TraceStart, const FSavedFrame& Frame, const FVector_NetQuantize& HitLocation, AWeapon* Weapon)
{
	if (HitSpearmanCharacter == nullptr) return FRewindResult();
	
	/* [Reserve HitBoxes of Current Frame] -> [MoveHitBoxes For Simulate Hit] -> [Return Result] -> [Reset HitBoxes to Reserved Current Frame] */
	FSavedFrame CurrentFrame;
	ReserveCurrentFrame(HitSpearmanCharacter, CurrentFrame);
	MoveHitBoxes(HitSpearmanCharacter, Frame);
	HitSpearmanCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UBoxComponent* HeadBox = HitSpearmanCharacter->HitBoxArray[0];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	FHitResult SimulateHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.2f;
	UWorld* World = GetWorld();
	if (World)
	{ /* Check Head Hit */
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(GetOwner());
		Params.AddIgnoredActor(Weapon);
		World->LineTraceSingleByChannel(SimulateHitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility, Params);

		if (SimulateHitResult.bBlockingHit)
		{ /* Head Hit, Reset to Reserved Current Frame's Hitbox Position */
			ResetHitBoxes(HitSpearmanCharacter, CurrentFrame);
			HitSpearmanCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			return FRewindResult{ true, true };
		}
		else
		{ /* Check Body Hit */
			for (UBoxComponent* HitBox : HitSpearmanCharacter->HitBoxArray)
			{
				if (HitBox)
				{
					HitBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
				}
			}
			World->LineTraceSingleByChannel(SimulateHitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility, Params);

			if (SimulateHitResult.bBlockingHit)
			{ /* Body Hit, Reset to Reserved Current Frame's Hitbox Position */
				ResetHitBoxes(HitSpearmanCharacter, CurrentFrame);
				HitSpearmanCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				return FRewindResult{ true, false };
			}
		}
	}

	ResetHitBoxes(HitSpearmanCharacter, CurrentFrame);
	HitSpearmanCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	return FRewindResult{ false, false };
}

void ULagCompensationComponent::ReserveCurrentFrame(ASpearmanCharacter* HitSpearmanCharacter, FSavedFrame& OutReservedFrame)
{ /* Reserve HitBoxes of Current Frame to restore original position */
	if (HitSpearmanCharacter == nullptr) return;

	for (UBoxComponent* Box : HitSpearmanCharacter->HitBoxArray)
	{
		if (Box)
		{
			FHitBox HitBox;
			HitBox.Location = Box->GetComponentLocation();
			HitBox.Rotation = Box->GetComponentRotation();
			HitBox.Extent = Box->GetScaledBoxExtent();
			OutReservedFrame.SavedHitBoxArray.Add(HitBox);
		}
	}
}

void ULagCompensationComponent::MoveHitBoxes(ASpearmanCharacter* HitSpearmanCharacter, const FSavedFrame& FrameToMove)
{
	if (HitSpearmanCharacter == nullptr) return;

	const TArray<UBoxComponent*>& HitSpearmanCharacterHitBoxArray = HitSpearmanCharacter->HitBoxArray;
	for (int32 idx = 0; idx < HitSpearmanCharacterHitBoxArray.Num(); idx++)
	{
		HitSpearmanCharacterHitBoxArray[idx]->SetWorldLocation(FrameToMove.SavedHitBoxArray[idx].Location);
		HitSpearmanCharacterHitBoxArray[idx]->SetWorldRotation(FrameToMove.SavedHitBoxArray[idx].Rotation);
		HitSpearmanCharacterHitBoxArray[idx]->SetBoxExtent(FrameToMove.SavedHitBoxArray[idx].Extent);
	}
}

void ULagCompensationComponent::ResetHitBoxes(ASpearmanCharacter* HitSpearmanCharacter, const FSavedFrame& ReservedFrame)
{
	if (HitSpearmanCharacter == nullptr) return;

	const TArray<UBoxComponent*>& HitSpearmanCharacterHitBoxArray = HitSpearmanCharacter->HitBoxArray;
	for (int32 idx = 0; idx < HitSpearmanCharacterHitBoxArray.Num(); idx++)
	{
		HitSpearmanCharacterHitBoxArray[idx]->SetWorldLocation(ReservedFrame.SavedHitBoxArray[idx].Location);
		HitSpearmanCharacterHitBoxArray[idx]->SetWorldRotation(ReservedFrame.SavedHitBoxArray[idx].Rotation);
		HitSpearmanCharacterHitBoxArray[idx]->SetBoxExtent(ReservedFrame.SavedHitBoxArray[idx].Extent);
		HitSpearmanCharacterHitBoxArray[idx]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ULagCompensationComponent::ShowSavedFrame(const FSavedFrame& Frame, const FColor& Color)
{
	for (const FHitBox& Box : Frame.SavedHitBoxArray)
	{
		DrawDebugBox(GetWorld(), Box.Location, Box.Extent, FQuat(Box.Rotation), Color, false, 4.f);
	}
}