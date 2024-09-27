// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanCharacterAnimInstance.h"
#include "SpearmanCharacter.h"
#include "Spearman/Weapon/Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Spearman/Weapon/Weapon.h"
#include "Spearman/SpearComponents/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/PoseSnapshot.h"


void USpearmanCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	SpearmanCharacter = Cast<ASpearmanCharacter>(TryGetPawnOwner());
}

void USpearmanCharacterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (SpearmanCharacter == nullptr)
	{
		SpearmanCharacter = Cast<ASpearmanCharacter>(TryGetPawnOwner());
	}
	if (SpearmanCharacter == nullptr)
		return;

	FVector Velocity = SpearmanCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = SpearmanCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = SpearmanCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquipped = SpearmanCharacter->IsWeaponEquipped();
	TIPState = SpearmanCharacter->GetTIPState();
	bIsDead = SpearmanCharacter->IsDead();

	// BlendSpace 동기화 부분
	FRotator AimRotation = SpearmanCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(SpearmanCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	// DeltaRotation : 이동 방향을 판정하기 위함, RInterpTo는 -180에서 180으로 최단거리 변경을 위함
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = SpearmanCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = 0.f;
	AO_Pitch = SpearmanCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && SpearmanCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"));
		FVector OutPosition;
		FRotator OutRotation;
		SpearmanCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
	}
}

void USpearmanCharacterAnimInstance::AnimNotify_AttackCollisionOn()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->TurnOnAttackCollision();
	}
}

void USpearmanCharacterAnimInstance::AnimNotify_AttackCollisionOff()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->TurnOffAttackCollision();
	}
}

void USpearmanCharacterAnimInstance::AnimNotify_SetStateIdle()
{
	if (SpearmanCharacter && SpearmanCharacter->HasAuthority())
	{
		SpearmanCharacter->GetCombat()->CombatState = ECombatState::ECS_Idle;
	}
}

void USpearmanCharacterAnimInstance::AnimNotify_HitReactEnd()
{
	if (SpearmanCharacter && SpearmanCharacter->HasAuthority())
	{
		SpearmanCharacter->GetCombat()->CombatState = ECombatState::ECS_Idle;
	}
}

void USpearmanCharacterAnimInstance::AnimNotify_DashEnd()
{
	if (SpearmanCharacter && SpearmanCharacter->HasAuthority())
	{
		SpearmanCharacter->GetCombat()->CombatState = ECombatState::ECS_Idle;
	}
}

void USpearmanCharacterAnimInstance::AnimNotify_ParriedEnd()
{
	if (SpearmanCharacter && SpearmanCharacter->HasAuthority())
	{
		SpearmanCharacter->GetCombat()->CombatState = ECombatState::ECS_Idle;
	}
}

void USpearmanCharacterAnimInstance::AnimNotify_StartDefense()
{
	if (SpearmanCharacter == nullptr || EquippedWeapon == nullptr) return;
	
	if (SpearmanCharacter->HasAuthority())
	{
		SpearmanCharacter->GetCombat()->CombatState = ECombatState::ECS_Defending;
	}
	
	EquippedWeapon->GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void USpearmanCharacterAnimInstance::AnimNotify_EndDefense()
{
	if (SpearmanCharacter == nullptr || EquippedWeapon == nullptr) return;

	if (SpearmanCharacter->HasAuthority())
	{
		SpearmanCharacter->GetCombat()->CombatState = ECombatState::ECS_Idle;
	}
	
	EquippedWeapon->GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
