// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Spearman/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Spearman/HUD/SpearmanHUD.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bCanDash);
	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SetHUDCrosshairs(DeltaTime);
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;

	Controller = (Controller == nullptr) ? Cast<ASpearmanPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		HUD = (HUD == nullptr) ? Cast<ASpearmanHUD>(Controller->GetHUD()) : HUD;

		if (HUD)
		{
			FHUDPackage HUDPackage;
			
			HUDPackage.CrosshairCircle = Character->GetCrosshairCircle();
			HUDPackage.CrosshairDot = Character->GetCrosshairDot();
			
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::DashButtonPressed_Implementation(const FVector& DashDirection)
{ /* Server Only */
	// TODO : SetCombatState : SuperArmor, Timer Cooldown
	if (DashDirection == FVector::ZeroVector || Character->GetMovementComponent()->IsFalling()) return;
	if (bCanDash == false) return;
	if (CombatState != ECombatState::ECS_Idle) return;

	float DotProduct = FVector::DotProduct(DashDirection, Character->GetActorForwardVector());
	if (EquippedWeapon && FMath::IsNearlyEqual(DotProduct, 0.f, 0.1f))
	{
		Dash(DashDirection);
	}
}

void UCombatComponent::Dash(const FVector& DashDirection)
{ /* Server Only */
	bCanDash = false;
	Character->LaunchCharacter(DashDirection * 4500.f, true, true);
	Character->GetWorldTimerManager().SetTimer(DashTimer, this, &UCombatComponent::SetDashCooldown, 2.f, false);
}

void UCombatComponent::MulticastSpearAttack_Implementation()
{
	if (Character && CombatState == ECombatState::ECS_Idle)
	{
		CombatState = ECombatState::ECS_Attacking;
		Character->PlaySpearAttackMontage();
	}
}

void UCombatComponent::ServerSpearAttack_Implementation()
{
	if (CombatState == ECombatState::ECS_Idle)
	{
		MulticastSpearAttack();
	}
}

void UCombatComponent::SetDashCooldown()
{
	bCanDash = true;
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{ /* Server Only */
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* RightHandWeaponSocket = Character->GetMesh()->GetSocketByName(FName("WeaponSocket"));
	if (RightHandWeaponSocket)
	{
		RightHandWeaponSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	EquippedWeapon->SetOwner(Character);
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{ /* Client Only */
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* RightHandWeaponSocket = Character->GetMesh()->GetSocketByName(FName("WeaponSocket"));
		if (RightHandWeaponSocket)
		{
			RightHandWeaponSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}
