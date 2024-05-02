// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "Spearman/Character/SpearmanCharacter.h"


UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;


}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();


}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Trouble Shooting : Buff 시작직전 DeltaTimeSum이 [0, 1) 값을 미리 가지므로 회복 타이밍이 다를 수 있음.
	// TODO : bHealing이 true가 된 뒤 DeltaTime 덧셈 시작.
	if (Character && Character->HasAuthority())
	{
		BuffTimeTickSum += DeltaTime;

		if (BuffTimeTickSum >= 1.f)
		{
			BuffTimeTickSum = 0.f;
			Heal(DeltaTime);
		}
	}
}

void UBuffComponent::StartHeal(float HealAmount, float BuffTime)
{ // server only, invoked from HealthPickup
	bHealing = true;
	TotalHealAmount = HealAmount;
	HealAmountPerSec = HealAmount / BuffTime;
	HealingTime = BuffTime;
}

void UBuffComponent::Heal(float DeltaTime)
{
	if (!bHealing || Character == nullptr || Character->IsDead()) return;
	
	const float HealAmountNow = FMath::Min(TotalHealAmount, HealAmountPerSec);
	const float NewHp = FMath::Clamp(Character->GetHp() + HealAmountNow, 0.f, Character->GetMaxHp());
	Character->SetHp(NewHp);
	Character->UpdateHUDHp();

	TotalHealAmount = FMath::Clamp(TotalHealAmount - HealAmountNow, 0.f, TotalHealAmount);

	if (FMath::IsNearlyZero(TotalHealAmount) || FMath::IsNearlyEqual(Character->GetHp(), Character->GetMaxHp()))
	{
		bHealing = false;
	}
}