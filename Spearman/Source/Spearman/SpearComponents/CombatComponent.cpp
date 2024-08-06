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
	
	SetHUDCrosshairs();
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UCombatComponent::SetHUDCrosshairs()
{
	if (Character == nullptr || Character->Controller == nullptr) return;

	Controller = Cast<ASpearmanPlayerController>(Character->Controller);

	if (Controller)
	{
		HUD = Cast<ASpearmanHUD>(Controller->GetHUD());

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
	
	CombatState = ECombatState::ECS_SuperArmor;

	// const float DotProduct = FVector::DotProduct(DashDirection, Character->GetActorForwardVector());
	MulticastDash(DashDirection);
}

void UCombatComponent::MulticastDash_Implementation(const FVector& DashDirection)
{
	bCanDash = false;
	
	bool bDashLeft = (DashDirection.Y < -0.5f) ? true : false;
	Character->PlayDashMontage(bDashLeft);

	Character->GetWorldTimerManager().SetTimer(DashTimer, this, &UCombatComponent::SetDashCooldown, 2.f, false);
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
