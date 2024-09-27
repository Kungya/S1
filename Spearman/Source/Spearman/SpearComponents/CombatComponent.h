// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Spearman/SpearmanTypes/CombatState.h"
#include "CombatComponent.generated.h"

class ASpearmanCharacter;
class ASpearmanPlayerController;
class ASpearmanHUD;
class AWeapon;
class USoundCue;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPEARMAN_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	friend class ASpearmanCharacter;
	UCombatComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetHUDCrosshairs();

	void EquipWeapon(AWeapon* WeaponToEquip);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastParried(ASpearmanCharacter* Opponent, FVector_NetQuantize Location);

	// 현재 캐릭터의 전투 상태, 이거 하나로 동작 사용가능 유무를 판단
	UPROPERTY(VisibleAnywhere)
	ECombatState CombatState = ECombatState::ECS_Idle;

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(Server, Reliable)
	void ServerDash(FVector_NetQuantize DashDirection);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDash(bool DashDirection);

	void DropEquippedWeapon();

	UFUNCTION(Server, Reliable)
	void ServerSpearAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpearAttack();

	UFUNCTION(Server, Reliable)
	void ServerThrust();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastThrust();

	UFUNCTION(Server, Reliable)
	void ServerStartDefense();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartDefense();

	UFUNCTION(Server, Reliable)
	void ServerEndDefense();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEndDefense();

	UFUNCTION()
	void OnRep_EquippedWeapon(AWeapon* UnEquippedWeapon);

private:
	UPROPERTY()
	ASpearmanCharacter* Character;
	
	UPROPERTY()
	ASpearmanPlayerController* SpearmanPlayerController;
	
	UPROPERTY()
	ASpearmanHUD* SpearmanHUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	AWeapon* DroppedWeapon;

	FTimerHandle DashTimer;

	void SetDashCooldown();

	UPROPERTY()
	bool bCanDash = true;

	UPROPERTY(EditAnywhere)
	USoundCue* ParryingSound;
	
public:

	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

};