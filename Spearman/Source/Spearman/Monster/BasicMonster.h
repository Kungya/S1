// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Spearman/Interfaces/WeaponHitInterface.h"
#include "BasicMonster.generated.h"

class UHitDamageWidget;

UCLASS()
class SPEARMAN_API ABasicMonster : public ACharacter, public IWeaponHitInterface
{
	GENERATED_BODY()

public:
	ABasicMonster();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnAttacked(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	void PlayHitMontage(FName Section, float PlayRate = 1.f);

	UFUNCTION()
	void StoreHitDamage(UHitDamageWidget* HitDamageToStore, FVector Location);

	UFUNCTION()
	void DestroyHitDamage(UHitDamageWidget* HitDamageToDestroy);

	void UpdateHitDamages();

	void ShowHpBar();
	void HideHpBar();

	void Die();

private:
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

	/*
	* Behavior Tree
	*/

	UPROPERTY(EditAnywhere, Category = "Behavior Tree")
	class UBehaviorTree* BehaviorTree;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void WeaponHit_Implementation(FHitResult HitResult) override;

	void ShowHitDamage(int32 Damage, FVector HitLocation, bool bHeadShot);

public:
	FORCEINLINE FString GetHeadBone() const { return HeadBone; }
	FORCEINLINE float GetHpRatio() const { return Hp / MaxHp; }
	FORCEINLINE UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }
};
