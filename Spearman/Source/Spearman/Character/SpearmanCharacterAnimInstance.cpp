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

	// BlendSpace ����ȭ �κ�
	FRotator AimRotation = SpearmanCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(SpearmanCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	// DeltaRotation : �̵� ������ �����ϱ� ����, RInterpTo�� -180���� 180���� �ִܰŸ� ������ ����
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

	// Weapon ����, TODO : AnimInstance���� ó���� �ϴ°� �´��� ?, �켱�� Anim_Notify������ ���⼭ ó��...
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

void USpearmanCharacterAnimInstance::AnimNotify_SetCanAttack()
{
	/*if (SpearmanCharacter->IsLocallyControlled())
		UE_LOG(LogTemp, Warning, TEXT("InClientSetCanAttack"));*/
	if (SpearmanCharacter && SpearmanCharacter->HasAuthority())
	{
		SpearmanCharacter->GetCombat()->SetCanAttack();
		//UE_LOG(LogTemp, Warning, TEXT("InServerSetCanAttack"));
	}
}
