// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Spearman/SpearmanTypes/CombatState.h"
#include "CombatComponent.generated.h"


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

	// 현재 캐릭터의 전투 상태, 이거 하나로 동작 사용가능 유무를 판단
	UPROPERTY(Replicated, VisibleAnywhere)
	ECombatState CombatState;

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(Server, Reliable)
	void DashButtonPressed(const FVector& DashDirection);

	void Dash(const FVector& DashDirection);

	// TODO : OnRep_EquipWeapon

	UFUNCTION(Server, Reliable)
	void ServerSpearAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpearAttack();

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void SetHUDCrosshairs(float DeltaTime);

private:
	class ASpearmanCharacter* Character;
	class ASpearmanPlayerController* Controller;
	class ASpearmanHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	FTimerHandle DashTimer;

	void SetDashCooldown();

	UPROPERTY(Replicated)
	bool bCanDash = true;


public:
	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

};