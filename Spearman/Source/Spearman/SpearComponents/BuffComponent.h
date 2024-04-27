// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPEARMAN_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuffComponent();
	friend class ASpearmanCharacter;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void StartHeal(float HealAmount, float BuffTime);
	void Heal(float DeltaTime);
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	class ASpearmanCharacter* Character;

	bool bHealing = false;
	float BuffTimeTickSum = 0.f;
	float TotalHealAmount = 0.f;
	float HealAmountPerSec = 0.f;
	float HealingTime = 0.f;
public:	
	

		
};
