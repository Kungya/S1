// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Spearman/SpearmanTypes/CombatState.h"
#include "CombatComponent.generated.h"

class ASpearmanCharacter;
class ASpearmanPlayerController;
class ASpearmanHUD;
class USoundCue;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPEARMAN_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	friend class ASpearmanCharacter;
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(class AWeapon* WeaponToEquip);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastParried(ASpearmanCharacter* Opponent, FVector_NetQuantize Location);

	// 현재 캐릭터의 전투 상태, 이거 하나로 동작 사용가능 유무를 판단
	UPROPERTY(Replicated, EditAnywhere)
	ECombatState CombatState = ECombatState::ECS_Idle;

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(Server, Reliable)
	void ServerDash(const FVector_NetQuantize DashDirection);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDash(const FVector& DashDirection);

	void DropEquippedWeapon();

	UFUNCTION(Server, Reliable)
	void ServerSpearAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpearAttack();

	UFUNCTION(Server, Reliable)
	void ServerThrust();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastThrust();

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void SetHUDCrosshairs(float DeltaTime);

private:
	UPROPERTY()
	ASpearmanCharacter* Character;
	
	UPROPERTY()
	ASpearmanPlayerController* Controller;
	
	UPROPERTY()
	ASpearmanHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	FTimerHandle DashTimer;

	void SetDashCooldown();

	UPROPERTY(Replicated)
	bool bCanDash = true;

	UPROPERTY(EditAnywhere)
	USoundCue* ParryingSound;


public:
	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

};