// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearmanCharacterAnimInstance.h"
#include "SpearmanCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Spearman/Weapon/Weapon.h"
#include "Spearman/SpearComponents/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"

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

	// Weapon 설정, TODO : AnimInstance에서 처리를 하는게 맞는지 ?, 우선은 Anim_Notify때문에 여기서 처리...
	EquippedWeapon = SpearmanCharacter->GetCombat()->GetEquippedWeapon();
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
	if (EquippedWeapon && EquippedWeapon->HasAuthority())
	{
		EquippedWeapon->TurnOnAttackCollision();
	}
}

void USpearmanCharacterAnimInstance::AnimNotify_AttackCollisionOff()
{
	if (EquippedWeapon && EquippedWeapon->HasAuthority())
	{
		EquippedWeapon->TurnOffAttackCollision();
	}
}

void USpearmanCharacterAnimInstance::AnimNotify_SetCanAttack()
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
