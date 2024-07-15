// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SpearmanAnimNotifyState.generated.h"

class ASpearmanCharacter;
class UCombatComponent;
class AWeapon;

UCLASS()
class SPEARMAN_API USpearmanAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:

private:
	UPROPERTY()
	ASpearmanCharacter* SpearmanCharacter;

	UPROPERTY()
	UCombatComponent* Combat;

	UPROPERTY()
	AWeapon* Weapon;


public:

};
