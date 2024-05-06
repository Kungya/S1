// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Spearman/Interfaces/WeaponHitInterface.h"
#include "BasicMonster.generated.h"

class UHitDamageWidget;
class AItem;

UCLASS()
class SPEARMAN_API ABasicMonster : public ACharacter, public IWeaponHitInterface
{
	GENERATED_BODY()

public:
	ABasicMonster();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
protected:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnAttacked(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	UFUNCTION()
	void StoreHitDamage(UHitDamageWidget* HitDamageToStore, FVector Location);

	UFUNCTION()
	void DestroyHitDamage(UHitDamageWidget* HitDamageToDestroy);

	void UpdateHitDamages();

	void ShowHpBar();
	void HideHpBar();

	UFUNCTION()
	void AggroSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void CombatRangeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void CombatRangeEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION()
		void AttackCollisionBoxBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

private:

	/*
	* Hit, Stats
	*/

	UPROPERTY(EditAnywhere, Category = Combat)
	class UParticleSystem* HitParticles;

	UPROPERTY(EditAnywhere, Category = Combat)
	float MaxHp = 75.f;

	UPROPERTY(ReplicatedUsing = OnRep_Hp, VisibleAnywhere)
	float Hp = 75.f;

	UFUNCTION()
	void OnRep_Hp();
	
	UPROPERTY(EditAnywhere, Category = Combat)
	FString HeadBone;

	/*
	* Widget
	*/

	UPROPERTY(VisibleAnywhere, Category = Widget)
	class UWidgetComponent* HitDamage;

	UPROPERTY(VisibleAnywhere, Category = Widget)
	UHitDamageWidget* HitDamageWidget;

	UPROPERTY(VisibleAnywhere, Category = Widget)
	class UWidgetComponent* HpBar;

	UPROPERTY(VisibleAnywhere, Category = Widget)
	class UHpBarWidget* HpBarWidget;

	// Map to store HitDamage Widgets and locations
	UPROPERTY(VisibleAnywhere, Category = Widget)
	TMap<UHitDamageWidget*, FVector> HitDamages;

	UPROPERTY(EditAnywhere, Category = Widget)
	float HitDamageDestroyTime = 1.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float HpBarDisplayTime = 4.f;

	FTimerHandle HpBarTimer;

	/*
	* Montage
	*/

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* DeathMontage;

	/*
	* Behavior Tree
	*/

	UPROPERTY(EditAnywhere, Category = "Behavior Tree")
	class UBehaviorTree* BehaviorTree;

	// Point for the monster to move to, in local space
	UPROPERTY(EditAnywhere, Category = "Behavior Tree", meta = (AllowPrivateAccess = "true", MakeEditWidget = "true"))
	FVector PatrolPoint;

	UPROPERTY(EditAnywhere, Category = "Behavior Tree", meta = (AllowPrivateAccess = "true", MakeEditWidget = "true"))
	FVector PatrolPoint2;

	class ABasicMonsterAIController* BasicMonsterAIController;

	/*
	* Combat
	*/
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	class USphereComponent* AggroSphere;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	bool bStunned = false;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float StunChance = 1.f;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	bool bInCombatRange;

	UPROPERTY(EditAnywhere, Category = "Combat")
	USphereComponent* CombatRangeSphere;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	class UBoxComponent* AttackCollisionBox;

	UPROPERTY()
	TSet<AActor*> HitSet;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float BaseDamage = 20.f;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	bool bCanAttack = true;

	FTimerHandle AttackWaitTimer;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackWaitTime = 1.f; 

	bool bDying = false;
	
	/*
	* Drop
	*/

	UPROPERTY(EditAnywhere, Category = "Drop")
	TSubclassOf<AItem> Item1Class;

	UPROPERTY(EditAnywhere, Category = "Drop")
	TSubclassOf<AItem> Item2Class;

	UPROPERTY(EditAnywhere, Category = "Drop")
	TSubclassOf<AItem> Item3Class;

	UPROPERTY(EditAnywhere, Category = "Drop")
	TSubclassOf<AItem> Item4Class;

	UPROPERTY(EditAnywhere, Category = "Drop")
	TSubclassOf<AItem> Item5Class;
public:
	virtual void WeaponHit_Implementation(FHitResult HitResult) override;

	void ShowHitDamage(int32 Damage, FVector HitLocation, bool bHeadShot);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastHit();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDeath();

	void SetStunned(bool Stunned);

	void TurnOnAttackCollision();
	void TurnOffAttackCollision();

	void SetCanAttack();
	void SetCanAttackTimer();

	void Death();

	void DropItems();

public:
	FORCEINLINE FString GetHeadBone() const { return HeadBone; }
	FORCEINLINE float GetHpRatio() const { return Hp / MaxHp; }
	FORCEINLINE UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }
};
