// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/Monster/BasicMonster.h"
#include "Spearman/Weapon/Weapon.h"
#include "Spearman/SpearComponents/HistoryComponent.h"
#include "Spearman/SpearComponents/CombatComponent.h"
#include "Spearman/Interfaces/RewindableInterface.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();

	SpearmanCharacter = Cast<ASpearmanCharacter>(GetOwner());
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (SpearmanCharacter == nullptr)
	{
		SpearmanCharacter = Cast<ASpearmanCharacter>(GetOwner());
	}
}

void ULagCompensationComponent::ServerRewindRequest_Implementation(ARewindableCharacter* HitRewindableCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* Weapon)
{ /* Server Only */
	if (HitRewindableCharacter == nullptr || Weapon == nullptr) return;

	FRewindResult RewindResult = Rewind(HitRewindableCharacter, TraceStart, HitLocation, HitTime, Weapon);
	if (RewindResult.bHit)
	{
		bool bHeadShot = false;
		float HitPartDamage = Weapon->GetDamage();

		if (RewindResult.bHeadShot)
		{
			bHeadShot = true;
			HitPartDamage = Weapon->GetHeadShotDamage();
		}

		const float Dist = FVector::Distance(SpearmanCharacter->GetActorLocation(), HitRewindableCharacter->GetActorLocation());

		FVector2D InRange(60.f, 240.f);
		FVector2D OutRange(HitPartDamage / 3.f, HitPartDamage);
		const float InDamage = FMath::GetMappedRangeValueClamped(InRange, OutRange, Dist);
		UGameplayStatics::ApplyDamage(HitRewindableCharacter, InDamage, SpearmanCharacter->Controller, Weapon, UDamageType::StaticClass());

		Weapon->MulticastHitEffect(HitRewindableCharacter, FMath::CeilToInt(InDamage), HitLocation, bHeadShot);

		UE_LOG(LogTemp, Warning, TEXT("Rewind Hit"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Rewind Miss"));
	}
}

FRewindResult ULagCompensationComponent::Rewind(ARewindableCharacter* HitRewindableCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* Weapon)
{
	if (HitRewindableCharacter == nullptr) return FRewindResult();

	UHistoryComponent* HitHistory = HitRewindableCharacter->GetHistory();
	if (HitHistory == nullptr || HitHistory->HistoricalBuffer.GetHead() == nullptr || HitHistory->HistoricalBuffer.GetTail() == nullptr) return FRewindResult();

	// Historical Buffer of HitRewindableCharacter, Not Attacker
	const TDoubleLinkedList<FSavedFrame>& Buffer = HitHistory->HistoricalBuffer;
	const float OldestTime = Buffer.GetTail()->GetValue().Time;
	const float NewestTime = Buffer.GetHead()->GetValue().Time;

	FSavedFrame FrameToSimulate;
	/* Can't rewind, Too High Ping */
	if (HitTime <= OldestTime) return FRewindResult();
	if (HitTime >= NewestTime)
	{ /* Don't have to Interp, use recent node (head) */
		FrameToSimulate = Buffer.GetHead()->GetValue();
		return SimulateHit(HitRewindableCharacter, TraceStart, FrameToSimulate, HitLocation, Weapon);
	}

	TDoubleLinkedList<FSavedFrame>::TDoubleLinkedListNode* HitTimeNextNode = Buffer.GetHead();
	TDoubleLinkedList<FSavedFrame>::TDoubleLinkedListNode* HitTimePrevNode = Buffer.GetHead();
	/* (Old) - [Frame] - [HitTimeNextNode] - (HitTime) - [HitTimePrevNode] - [Frame] - (Recent) */

	/* Search until Next of HitTime	*/
	while (HitTimeNextNode->GetValue().Time > HitTime)
	{
		if (HitTimeNextNode->GetNextNode() == nullptr) break;

		HitTimeNextNode = HitTimeNextNode->GetNextNode();

		/* HitTimePrevNode continuously follow HitTimeNextNode if HitTimeNextNode's Time has not yet pass HitTime */
		if (HitTimeNextNode->GetValue().Time > HitTime)
		{
			HitTimePrevNode = HitTimeNextNode;
		}
	}
	
	FrameToSimulate = GetInterpFrame(HitTimeNextNode->GetValue(), HitTimePrevNode->GetValue(), HitTime);

	return SimulateHit(HitRewindableCharacter, TraceStart, FrameToSimulate, HitLocation, Weapon);
}

FRewindResult ULagCompensationComponent::SimulateHit(ARewindableCharacter* HitRewindableCharacter, const FVector_NetQuantize& TraceStart, const FSavedFrame& Frame, const FVector_NetQuantize& HitLocation, AWeapon* Weapon)
{
	if (HitRewindableCharacter == nullptr) return FRewindResult();
	
	IRewindableInterface* RewindableInterface = Cast<IRewindableInterface>(HitRewindableCharacter);
	check(RewindableInterface != nullptr);

	FSavedFrame CurrentFrame;
	/* Reserve Current Frame's HItBoxes before Rewind*/
	ReserveCurrentFrame(RewindableInterface, CurrentFrame);
	MoveHitBoxes(RewindableInterface, Frame);
	HitRewindableCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UBoxComponent* HeadBox = HitRewindableCharacter->HitBoxArray[0];
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
		{
			ResetHitBoxes(RewindableInterface, CurrentFrame);
			HitRewindableCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			return FRewindResult{ true, true };
		}
		else
		{ /* Check Body Hit */
			for (UBoxComponent* HitBox : HitRewindableCharacter->HitBoxArray)
			{
				if (HitBox)
				{
					HitBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
				}
			}
			World->LineTraceSingleByChannel(SimulateHitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility, Params);

			if (SimulateHitResult.bBlockingHit)
			{
				ResetHitBoxes(RewindableInterface, CurrentFrame);
				HitRewindableCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				return FRewindResult{ true, false };
			}
		}
	}

	ResetHitBoxes(RewindableInterface, CurrentFrame);
	HitRewindableCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	return FRewindResult{ false, false };
}

void ULagCompensationComponent::ShowSavedFrame(const FSavedFrame& Frame, const FColor& Color)
{
	for (const FHitBox& Box : Frame.SavedHitBoxArray)
	{
		DrawDebugBox(GetWorld(), Box.Location, Box.Extent, FQuat(Box.Rotation), Color, false, 4.f);
	}
}

void ULagCompensationComponent::ServerRewindRequestForParrying_Implementation(ARewindableActor* HitRewindableActor, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* Weapon)
{
	if (HitRewindableActor == nullptr || Weapon == nullptr) return;

	bool bParried = Rewind(HitRewindableActor, TraceStart, HitLocation, HitTime, Weapon);

	if (bParried)
	{
		AWeapon* HitWeaponToParried = Cast<AWeapon>(HitRewindableActor);
		if (HitWeaponToParried)
		{
			HitWeaponToParried->CheckOwnerSpearmanCharacterIsValid();
			HitWeaponToParried->GetOwnerSpearmanCharacter()->GetCombat()->MulticastParried(SpearmanCharacter, HitLocation);
		}

		UE_LOG(LogTemp, Warning, TEXT("Rewind Success : Parried"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Rewind Miss : Not Parried"));
	}
}

bool ULagCompensationComponent::Rewind(ARewindableActor* HitRewindableActor, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* Weapon)
{
	if (HitRewindableActor == nullptr) return false;

	UHistoryComponent* HitHistory = HitRewindableActor->GetHistory();
	if (HitHistory == nullptr || HitHistory->HistoricalBuffer.GetHead() == nullptr || HitHistory->HistoricalBuffer.GetTail() == nullptr) return false;

	// Historical Buffer of ARewindableActor* HitWeapon
	const TDoubleLinkedList<FSavedFrame>& Buffer = HitHistory->HistoricalBuffer;
	const float OldestTime = Buffer.GetTail()->GetValue().Time;
	const float NewestTime = Buffer.GetHead()->GetValue().Time;
	
	FSavedFrame FrameToSimulate;
	/* Too Hit Ping */
	if (HitTime <= OldestTime) return false;
	/* Don't have to Interp, use recent node */
	if (HitTime >= NewestTime)
	{
		FrameToSimulate = Buffer.GetHead()->GetValue();
		return SimulateHit(HitRewindableActor, TraceStart, FrameToSimulate, HitLocation, Weapon);
	}

	TDoubleLinkedList<FSavedFrame>::TDoubleLinkedListNode* HitTimeNextNode = Buffer.GetHead();
	TDoubleLinkedList<FSavedFrame>::TDoubleLinkedListNode* HitTimePrevNode = Buffer.GetHead();

	while (HitTimeNextNode->GetValue().Time > HitTime)
	{
		if (HitTimeNextNode->GetNextNode() == nullptr) break;

		HitTimeNextNode = HitTimeNextNode->GetNextNode();

		if (HitTimeNextNode->GetValue().Time > HitTime)
		{
			HitTimePrevNode = HitTimeNextNode;
		}
	}

	FrameToSimulate = GetInterpFrame(HitTimeNextNode->GetValue(), HitTimePrevNode->GetValue(), HitTime);

	return SimulateHit(HitRewindableActor, TraceStart, FrameToSimulate, HitLocation, Weapon);
}

bool ULagCompensationComponent::SimulateHit(ARewindableActor* HitRewindableActor, const FVector_NetQuantize& TraceStart, const FSavedFrame& Frame, const FVector_NetQuantize& HitLocation, AWeapon* Weapon)
{
	if (HitRewindableActor == nullptr) return false;

	IRewindableInterface* RewindableInterface = Cast<IRewindableInterface>(HitRewindableActor);
	check(RewindableInterface != nullptr);

	FSavedFrame CurrentFrame;
	ReserveCurrentFrame(HitRewindableActor, CurrentFrame);
	MoveHitBoxes(RewindableInterface, Frame);
	HitRewindableActor->GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UBoxComponent* WeaponBox = HitRewindableActor->HitBoxArray[0];
	WeaponBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	FHitResult SimulateHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.2f;
	UWorld* World = GetWorld();
	if (World)
	{
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(GetOwner());
		Params.AddIgnoredActor(Weapon);
		World->LineTraceSingleByChannel(SimulateHitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility, Params);

		if (SimulateHitResult.bBlockingHit)
		{
			ResetHitBoxes(RewindableInterface, CurrentFrame);
			HitRewindableActor->GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			return true;
		}
	}

	ResetHitBoxes(RewindableInterface, CurrentFrame);
	HitRewindableActor->GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
	return false;
}

FSavedFrame ULagCompensationComponent::GetInterpFrame(const FSavedFrame& Next, const FSavedFrame& Prev, float HitTime)
{ /* Get Similar HitBox by Interpolating between Next and Prev */
	const float Dist = (Prev.Time - Next.Time);
	const float InterpRatio = FMath::Clamp(((HitTime - Next.Time) / Dist), 0.f, 1.f);

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

void ULagCompensationComponent::ReserveCurrentFrame(IRewindableInterface* HitRewindableInterface, FSavedFrame& OutReservedFrame)
{
	if (HitRewindableInterface == nullptr) return;

	const TArray<UBoxComponent*>& HitBoxArray = HitRewindableInterface->GetHitBoxArray();

	for (UBoxComponent* Box : HitBoxArray)
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

void ULagCompensationComponent::MoveHitBoxes(IRewindableInterface* HitRewindableInterface, const FSavedFrame& FrameToMove)
{
	if (HitRewindableInterface == nullptr) return;

	const TArray<UBoxComponent*>& HitActorHitBoxArray = HitRewindableInterface->GetHitBoxArray();

	for (int32 idx = 0; idx < HitActorHitBoxArray.Num(); idx++)
	{
		UBoxComponent* Box = HitActorHitBoxArray[idx];
		if (Box)
		{
			Box->SetWorldLocation(FrameToMove.SavedHitBoxArray[idx].Location);
			Box->SetWorldRotation(FrameToMove.SavedHitBoxArray[idx].Rotation);
			Box->SetBoxExtent(FrameToMove.SavedHitBoxArray[idx].Extent);
		}
	}
}

void ULagCompensationComponent::ResetHitBoxes(IRewindableInterface* HitRewindableInterface, const FSavedFrame& ReservedFrame)
{
	if (HitRewindableInterface == nullptr) return;

	const TArray<UBoxComponent*>& HitActorHitBoxArray = HitRewindableInterface->GetHitBoxArray();

	for (int32 idx = 0; idx < HitActorHitBoxArray.Num(); idx++)
	{
		UBoxComponent* Box = HitActorHitBoxArray[idx];
		if (Box)
		{
			Box->SetWorldLocation(ReservedFrame.SavedHitBoxArray[idx].Location);
			Box->SetWorldRotation(ReservedFrame.SavedHitBoxArray[idx].Rotation);
			Box->SetBoxExtent(ReservedFrame.SavedHitBoxArray[idx].Extent);
			Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}