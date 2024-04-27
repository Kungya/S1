// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/SpearComponents/BuffComponent.h"

AHealthPickup::AHealthPickup()
{
}

void AHealthPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{ // server only
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ASpearmanCharacter* SpearmanCharacter = Cast<ASpearmanCharacter>(OtherActor);
	if (SpearmanCharacter)
	{
		UBuffComponent* Buff = SpearmanCharacter->GetBuff();
		
		if (Buff)
		{
			Buff->StartHeal(HealAmount, HealingTime);
		}
	}

	Destroy();
}
