// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicMonsterAnimInstance.h"
#include "BasicMonster.h"

void UBasicMonsterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BasicMonster = Cast<ABasicMonster>(TryGetPawnOwner());
}

void UBasicMonsterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (BasicMonster == nullptr)
	{
		BasicMonster = Cast<ABasicMonster>(TryGetPawnOwner());
	}
	if (BasicMonster == nullptr) return;

	FVector Velocity = BasicMonster->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();
}

void UBasicMonsterAnimInstance::AnimNotify_StunFinish()
{
	if (BasicMonster && BasicMonster->HasAuthority())
	{
		BasicMonster->SetStunned(false);
		UE_LOG(LogTemp, Warning, TEXT("Stun Finish"));
	}
}

void UBasicMonsterAnimInstance::AnimNotify_AttackCollisionOn()
{
	if (BasicMonster && BasicMonster->HasAuthority())
	{
		BasicMonster->TurnOnAttackCollision();
	}
}

void UBasicMonsterAnimInstance::AnimNotify_AttackCollisionOff()
{
	if (BasicMonster && BasicMonster->HasAuthority())
	{
		BasicMonster->TurnOffAttackCollision();
	}
}

void UBasicMonsterAnimInstance::AnimNotify_EndDeath()
{
	if (BasicMonster && BasicMonster->HasAuthority())
	{ // server측에서만 destroy를 해도 되는지?
		BasicMonster->Destroy();
	}
}
