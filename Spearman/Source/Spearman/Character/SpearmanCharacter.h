// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Spearman/SpearmanTypes/TurnInPlace.h"
#include "Spearman/Interfaces/WeaponHitInterface.h"
#include "SpearmanCharacter.generated.h"

UCLASS()
class SPEARMAN_API ASpearmanCharacter : public ACharacter, public IWeaponHitInterface
{
	GENERATED_BODY()

public:
	friend class UCombatComponent;
	friend class UInventoryComponent;
	ASpearmanCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void OnRep_ReplicatedMovement() override;
	void UpdateHUDHp();

	void Death();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDeath();

	virtual void WeaponHit_Implementation(FHitResult HitResult) override;

	// Disable Key Input if Character Dies or Match is Ended...
	UPROPERTY(Replicated)
	bool bDisableKeyInput = false;
protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	virtual void Jump() override;
	void EquipButtonPressed();

	void InteractButtonPressed();
	void Interact();
	
	UFUNCTION(Server, Reliable)
	void ServerInteract();

	void InventoryButtonPressed();
	void InventoryButtonReleased();

	void HideCameraIfCharacterTooClose();
	void CalculateAO_Pitch();
	void TurnInPlace();

	void PlaySpearAttackMontage();
	void PlayHitReactMontage();
	void PlayDeathMontage();

	UFUNCTION()
	void OnAttacked(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	void ShowHitDamage(bool bShowHitDamageWidget);
	void HideHitDamage();

	void ShowHpBar();
	void HideHpBar();

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

	/*
	* Crosshair
	*/

	UPROPERTY(EditAnywhere, Category = Crosshair)
	class UTexture2D* CrosshairCircle;

	UPROPERTY(EditAnywhere, Category = Crosshair)
	UTexture2D* CrosshairDot;

	/* 
	* Widget
	*/

	UPROPERTY(VisibleAnywhere, Category = Widget)
	class UWidgetComponent* HitDamage;

	UPROPERTY(VisibleAnywhere, Category = Widget)
	class UHitDamageWidget* HitDamageWidget;

	FTimerHandle HitDamageTimerHandle;

	// Overhead HpBar; on Enemy
	UPROPERTY(VisibleAnywhere, Category = Widget)
	class UWidgetComponent* HpBar;

	UPROPERTY(VisibleAnywhere, Category = Widget)
	class UHpBarWidget* HpBarWidget;

	UPROPERTY(EditAnywhere, Category = Combat)
	float HpBarDisplayTime = 4.f;

	FTimerHandle HpBarTimer;
	
	/*
	* Spearman Components
	*/

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UInventoryComponent* Inventory;

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

	UPROPERTY(EditAnywhere, Category = Combat)
	float MaxHp = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Hp, VisibleAnywhere, Category = Combat)
	float Hp = 100.f;

	UFUNCTION()
	void OnRep_Hp(float LastHp);

	UPROPERTY(EditAnywhere, Category = Combat)
	FString HeadBone;

	UPROPERTY()
	class ASpearmanPlayerController* SpearmanPlayerController;

	bool bDeath = false;

	FTimerHandle DeathTimer;

	UPROPERTY(EditDefaultsOnly)
	float DeathDelay = 0.5;

	void DeathTimerFinished();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UParticleSystem* HitParticles;


public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurnInPlace GetTIPState() const { return TIPState; }
	FORCEINLINE UTexture2D* GetCrosshairCircle() const { return CrosshairCircle; }
	FORCEINLINE UTexture2D* GetCrosshairDot() const { return CrosshairDot; }
	FORCEINLINE bool IsDead() const { return bDeath; }
	FORCEINLINE void SetHp(float NewHp) { Hp = NewHp; }
	FORCEINLINE float GetHp() const { return Hp; }
	FORCEINLINE float GetMaxHp() const { return MaxHp; }
	FORCEINLINE float GetHpRatio() const { return Hp / MaxHp; }
	FORCEINLINE FString GetHeadBone() const { return HeadBone; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	FORCEINLINE UInventoryComponent* GetInventory() const { return Inventory; }
};