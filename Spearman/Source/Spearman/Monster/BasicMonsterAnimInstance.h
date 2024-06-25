// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BasicMonsterAnimInstance.generated.h"

class ABasicMonster;

/**
 * 
 */
UCLASS()
class SPEARMAN_API UBasicMonsterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

	UFUNCTION()
	void AnimNotify_StunFinish();

	UFUNCTION()
	void AnimNotify_AttackCollisionOn();

	UFUNCTION()
	void AnimNotify_AttackCollisionOff();

	UFUNCTION()
	void AnimNotify_EndDeath();
	
	
private:
	UPROPERTY(BlueprintReadOnly, Category = BasicMonster, meta = (AllowPrivateAccess = "true"))
	ABasicMonster* BasicMonster;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float Speed;
};
