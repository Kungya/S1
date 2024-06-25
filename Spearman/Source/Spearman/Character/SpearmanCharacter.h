// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Spearman/Character/RewindableCharacter.h"
#include "Spearman/SpearmanTypes/TurnInPlace.h"
#include "Spearman/Interfaces/WeaponHitInterface.h"
#include "SpearmanCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USceneCaptureComponent2D;
class UPaperSpriteComponent;
class UTextureRenderTarget2D;
class AWeapon;
class UTexture2D;
class UWidgetComponent;
class UHitDamageWidget;
class UHpBarWidget;
class UCombatComponent;
class UBuffComponent;
class UInventoryComponent;
class ULagCompensationComponent;
class UHistoryComponent;
class UAnimMontage;
class ASpearmanPlayerController;
class UParticleSystem;
class UBoxComponent;
class UCapsuleComponent;

UCLASS()
class SPEARMAN_API ASpearmanCharacter : public ARewindableCharacter, public IWeaponHitInterface
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

	virtual void WeaponHit_Implementation(int32 Damage, FVector_NetQuantize HitPoint, bool bHeadShot) override;

	// Disable Key Input if Character Dies or Match is Ended...
	UPROPERTY(Replicated)
	bool bDisableKeyInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Minimap)
	UTextureRenderTarget2D* RenderTargetMinimap;

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

	void TakeDamageIfNotInBlueZone();


private:

	/*
	* Camera
	*/

	UPROPERTY(VisibleAnywhere, Category = Camera)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	UCameraComponent* FollowCamera;

	/*
	* Minimap
	*/

	UPROPERTY(VisibleAnywhere, Category = Minimap)
	USpringArmComponent* MinimapSpringArm;

	UPROPERTY(VisibleAnywhere, Category = Minimap)
	UPaperSpriteComponent* MinimapCursor;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Minimap, meta = (AllowPrivateAccess))
	USceneCaptureComponent2D* MinimapSceneCapture;

	UPROPERTY (ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	/*
	* Crosshair
	*/

	UPROPERTY(EditAnywhere, Category = Crosshair)
	UTexture2D* CrosshairCircle;

	UPROPERTY(EditAnywhere, Category = Crosshair)
	UTexture2D* CrosshairDot;

	/* 
	* Widget
	*/

	UPROPERTY(VisibleAnywhere, Category = Widget)
	UWidgetComponent* HitDamage;

	UPROPERTY(VisibleAnywhere, Category = Widget)
	UHitDamageWidget* HitDamageWidget;

	FTimerHandle HitDamageTimerHandle;

	// Overhead HpBar; on Enemy
	UPROPERTY(VisibleAnywhere, Category = Widget)
	UWidgetComponent* HpBar;

	UPROPERTY(VisibleAnywhere, Category = Widget)
	UHpBarWidget* HpBarWidget;

	UPROPERTY(EditAnywhere, Category = Combat)
	float HpBarDisplayTime = 4.f;

	FTimerHandle HpBarTimer;
	
	/*
	* ActorComponents
	*/

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "ActorComponent")
	UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere, Category = "ActorComponent")
	UBuffComponent* Buff;

	UPROPERTY(VisibleAnywhere, Category = "ActorComponent")
	ULagCompensationComponent* LagCompensation;

	//UPROPERTY(VisibleAnywhere, Category = "ActorComponent")
	//UHistoryComponent* History;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "ActorComponent")
	UInventoryComponent* Inventory;

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

	/* Enum class for Turn In Place in AnimBP */
	ETurnInPlace TIPState;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* SpearAttackMontage;

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
	ASpearmanPlayerController* SpearmanPlayerController;

	bool bDeath = false;

	FTimerHandle DeathTimer;

	UPROPERTY(EditDefaultsOnly)
	float DeathDelay = 0.5;

	void DeathTimerFinished();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* HitParticles;

	/*
	* BlueZone
	*/

	/* false : apply damage */
	UPROPERTY(ReplicatedUsing = OnRep_bIsInBlueZone, VisibleAnywhere)
	bool bIsInBlueZone = true;

	UFUNCTION()
	void OnRep_bIsInBlueZone();

	FTimerHandle BlueZoneTimerHandle;

	/*
	* Hit Box
	*/

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* head;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* pelvis;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* spine_02;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* spine_03;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* upperarm_l;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* upperarm_r;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* lowerarm_l;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* lowerarm_r;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* hand_l;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* hand_r;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* thigh_l;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* thigh_r;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* calf_l;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* calf_r;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* foot_l;

	UPROPERTY(EditAnywhere, Category = "Hit Box")
	UBoxComponent* foot_r;

	/* Test for Rewind*/
	FTimerHandle TestTimer;
	float TimerVector = 1.f;
	void TestToggleVector() { TimerVector *= -1.f; };
	bool bMove = false;
	void TriggerMove() { bMove = true; };

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
	FORCEINLINE void SetHp(const float NewHp) { Hp = NewHp; }
	FORCEINLINE float GetHp() const { return Hp; }
	FORCEINLINE float GetMaxHp() const { return MaxHp; }
	FORCEINLINE float GetHpRatio() const { return Hp / MaxHp; }
	FORCEINLINE FString GetHeadBone() const { return HeadBone; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	FORCEINLINE UInventoryComponent* GetInventory() const { return Inventory; }
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
	FORCEINLINE void SetbIsInBlueZone(const bool NewbIsInBlueZone) { bIsInBlueZone = NewbIsInBlueZone; };
	FORCEINLINE UTextureRenderTarget2D* GetRenderTargetMinimap() const { return RenderTargetMinimap; }
};