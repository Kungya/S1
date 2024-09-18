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
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UCombatComponent::SetHUDCrosshairs()
{
	if (Character == nullptr || Character->Controller == nullptr) return;

	SpearmanPlayerController = Cast<ASpearmanPlayerController>(Character->Controller);

	if (Character->Controller)
	{
		SpearmanHUD = Cast<ASpearmanHUD>(SpearmanPlayerController->GetHUD());

		if (SpearmanHUD)
		{
			FHUDPackage HUDPackage;
			
			HUDPackage.CrosshairCircle = Character->GetCrosshairCircle();
			HUDPackage.CrosshairDot = Character->GetCrosshairDot();
			
			SpearmanHUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::ServerDash_Implementation(FVector_NetQuantize DashDirection)
{ /* Server Only */
	// TODO : SetCombatState : SuperArmor, Timer Cooldown
	if (Character == nullptr || Character->GetMovementComponent()->IsFalling()) return;
	if (bCanDash == false || EquippedWeapon == nullptr) return;
	if (CombatState != ECombatState::ECS_Idle) return;
	
	bCanDash = false;
	CombatState = ECombatState::ECS_SuperArmor;
	Character->GetWorldTimerManager().SetTimer(DashTimer, this, &UCombatComponent::SetDashCooldown, 2.f, false);

	bool bDashLeft = (DashDirection.Y < -0.5f) ? true : false;
	MulticastDash(bDashLeft);
}

void UCombatComponent::MulticastDash_Implementation(bool DashDirection)
{
	Character->PlayDashMontage(DashDirection);
}

void UCombatComponent::MulticastParried_Implementation(ASpearmanCharacter* Opponent, FVector_NetQuantize Location)
{ /* Two Spearmans' MulticastParried at once */
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), ParryingSound, Location);
	
	if (Opponent)
	{
		Opponent->PlayParriedMontage();
	}
	
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
		CombatState = ECombatState::ECS_Attacking;
		MulticastSpearAttack();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't Attack"));
	}
}

void UCombatComponent::MulticastSpearAttack_Implementation()
{
	if (Character)
	{
		Character->PlaySpearAttackMontage();
	}
}

void UCombatComponent::ServerThrust_Implementation()
{
	if (CombatState == ECombatState::ECS_Idle)
	{
		CombatState = ECombatState::ECS_Attacking;
		MulticastThrust();
	}
}

void UCombatComponent::MulticastThrust_Implementation()
{
	if (Character)
	{
		Character->PlayThrustMontage();
	}
}

void UCombatComponent::ServerStartDefense_Implementation()
{
	if (CombatState == ECombatState::ECS_Idle)
	{
		MulticastStartDefense();
	}
}

void UCombatComponent::MulticastStartDefense_Implementation()
{
	if (Character)
	{
		Character->PlayStartDefenseMontage();
	}
}

void UCombatComponent::ServerEndDefense_Implementation()
{
	if (CombatState == ECombatState::ECS_Defending && Character->GetIsPlayingDefenseMontage())
	{
		MulticastEndDefense();
	}
}

void UCombatComponent::MulticastEndDefense_Implementation()
{
	if (Character)
	{
		Character->PlayEndDefenseMontage();
	}
}

void UCombatComponent::SetDashCooldown()
{
	bCanDash = true;
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{ /* Server Only */
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	if (EquippedWeapon)
	{ // Swap (Equip and UnEquip)
		ASpearmanCharacter::NotifySwapWeapon.Broadcast(Character, WeaponToEquip, EquippedWeapon);
	}
	else if (!EquippedWeapon)
	{ // Equip Only
		ASpearmanCharacter::NotifySwapWeapon.Broadcast(Character, WeaponToEquip, nullptr);
	}

	DropEquippedWeapon();
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	CombatState = ECombatState::ECS_Idle;
	
	const USkeletalMeshSocket* RightHandWeaponSocket = Character->GetMesh()->GetSocketByName(FName("WeaponSocket"));
	if (RightHandWeaponSocket)
	{
		RightHandWeaponSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->GetWeaponMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	EquippedWeapon->GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
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
