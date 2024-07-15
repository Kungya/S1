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
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

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

void UCombatComponent::ServerDash_Implementation(FVector_NetQuantize DashDirection)
{ /* Server Only */
	// TODO : SetCombatState : SuperArmor, Timer Cooldown
	if (Character->GetMovementComponent()->IsFalling()) return;
	if (bCanDash == false || EquippedWeapon == nullptr) return;
	if (CombatState != ECombatState::ECS_Idle) return;
	
	// const float DotProduct = FVector::DotProduct(DashDirection, Character->GetActorForwardVector());
	MulticastDash(DashDirection);
}

void UCombatComponent::MulticastDash_Implementation(const FVector& DashDirection)
{
	bCanDash = false;
	CombatState = ECombatState::ECS_SuperArmor;
	
	bool bDashLeft = (DashDirection.Y < -0.5f) ? true : false;
	Character->PlayDashMontage(bDashLeft);

	Character->GetWorldTimerManager().SetTimer(DashTimer, this, &UCombatComponent::SetDashCooldown, 2.f, false);
}

void UCombatComponent::MulticastParried_Implementation(FVector_NetQuantize Location)
{
	CombatState = ECombatState::ECS_Stunned;

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), ParryingSound, Location);
	Character->PlayParriedMontage();
}

void UCombatComponent::DropEquippedWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::ServerSpearAttack_Implementation()
{
	if (CombatState == ECombatState::ECS_Idle)
	{
		MulticastSpearAttack();
	}
}

void UCombatComponent::MulticastSpearAttack_Implementation()
{
	if (Character && CombatState == ECombatState::ECS_Idle)
	{
		CombatState = ECombatState::ECS_Attacking;
		Character->PlaySpearAttackMontage();
	}
}

void UCombatComponent::ServerThrust_Implementation()
{
	if (CombatState == ECombatState::ECS_Idle)
	{
		MulticastThrust();
	}
}

void UCombatComponent::MulticastThrust_Implementation()
{
	if (CombatState == ECombatState::ECS_Idle)
	{
		CombatState = ECombatState::ECS_Attacking;
		Character->PlayThrustMontage();
	}
}

void UCombatComponent::SetDashCooldown()
{
	bCanDash = true;
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{ /* Server Only */
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	DropEquippedWeapon();
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	
	const USkeletalMeshSocket* RightHandWeaponSocket = Character->GetMesh()->GetSocketByName(FName("WeaponSocket"));
	if (RightHandWeaponSocket)
	{
		RightHandWeaponSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->GetWeaponMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	EquippedWeapon->GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
	/* for Parring in Server Rewind */
	// TODO : dont naive Add for HitBox (if equipweapon many)
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		const USkeletalMeshSocket* RightHandWeaponSocket = Character->GetMesh()->GetSocketByName(FName("WeaponSocket"));
		if (RightHandWeaponSocket)
		{
			RightHandWeaponSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}
	}
}
