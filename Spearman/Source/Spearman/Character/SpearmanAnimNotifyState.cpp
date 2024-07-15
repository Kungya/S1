// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanAnimNotifyState.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include <Spearman/SpearComponents/CombatComponent.h>
#include "Spearman/Weapon/Weapon.h"


void USpearmanAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp == nullptr) return;
	
	SpearmanCharacter = (SpearmanCharacter == nullptr) ? Cast<ASpearmanCharacter>(MeshComp->GetOwner()) : SpearmanCharacter;
	
	if (SpearmanCharacter)
	{
		Combat = (Combat == nullptr) ? SpearmanCharacter->GetCombat() : Combat;
		
		if (Combat)
		{
			Weapon = (Weapon == nullptr) ? Combat->GetEquippedWeapon() : Weapon;
		}
	}
}

void USpearmanAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (Weapon)
	{
		// Weapon->AnimStateAttack();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Something fault"));
	}
}

void USpearmanAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);


}