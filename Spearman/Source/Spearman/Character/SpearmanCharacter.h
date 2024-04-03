// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Spearman/SpearmanTypes/TurnInPlace.h"
#include "SpearmanCharacter.generated.h"

UCLASS()
class SPEARMAN_API ASpearmanCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	friend class UCombatComponent;
	ASpearmanCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void OnRep_ReplicatedMovement() override;
	// TODO : 기능이 더 추가된다면 Rep에서 Multicast로 변경
	void Death();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDeath();

	void ShowHitDamage(bool bShowHitDamageWidget);
	void HideHitDamage();

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	virtual void Jump() override;
	void EquipButtonPressed();

	void HideCameraIfCharacterTooClose();
	void CalculateAO_Pitch();
	void TurnInPlace();

	void PlaySpearAttackMontage();
	void PlayHitReactMontage();
	void PlayDeathMontage();

	UFUNCTION()
	void OnAttacked(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	
	void UpdateHUDHp();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	// 땅에 떨어진 무기와 겹쳤을 때의 Weapon
	UPROPERTY (ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, Category = Widget)
	class UWidgetComponent* HitDamage;

	UPROPERTY(VisibleAnywhere, Category = Widget)
	class UHitDamageWidget* HitDamageWidget;

	FTimerHandle HitDamageTimerHandle;

	/*
	* Spearman Components
	*/

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();
	
	void DashButtonPressed();
	void AttackButtonPressed();
	
	/*
	* Animation
	*/

	float AO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	// AnimBP에서 일정 틱마다 TurnInPlace 애니메이션을 실행할건지 상태를 나타냄
	ETurnInPlace TIPState;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* SpearAttackMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* DeathMontage;

	float TurnThreshold = 15.f;
	FRotator RotationPrevFrame;
	FRotator RotationNowFrame;
	float YawOffset;
	float TimeSinceLastMovementReplication;

	// HP Stat

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHp = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Hp, VisibleAnywhere, Category = "Player Stats")
	float Hp = 100.f;

	UFUNCTION()
	void OnRep_Hp(float LastHp);

	UPROPERTY()
	class ASpearmanPlayerController* SpearmanPlayerController;

	bool bDeath = false;

	FTimerHandle DeathTimer;

	UPROPERTY(EditDefaultsOnly)
	float DeathDelay = 0.5;

	void DeathTimerFinished();


public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurnInPlace GetTIPState() const { return TIPState; }
	FORCEINLINE bool IsDead() const { return bDeath; }
	FORCEINLINE float GetHp() const { return Hp; }
	FORCEINLINE float GetMaxHp() const { return MaxHp; }
};