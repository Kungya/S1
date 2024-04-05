// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Spearman/Interfaces/WeaponHitInterface.h"
#include "BasicMonster.generated.h"

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

	/*
	* Widget
	*/

	UPROPERTY(VisibleAnywhere, Category = Widget)
	class UWidgetComponent* HpBar;

	UPROPERTY(VisibleAnywhere, Category = Widget)
	class UHpBarWidget* HpBarWidget;

	UPROPERTY(EditAnywhere, Category = Combat)
	FString HeadBone;

	UPROPERTY(EditAnywhere, Category = Combat)
	float HpBarDisplayTime = 4.f;

	FTimerHandle HpBarTimer;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void WeaponHit_Implementation(FHitResult HitResult) override;

public:
	FORCEINLINE FString GetHeadBone() const { return HeadBone; }
	FORCEINLINE float GetHpRatio() const { return Hp / MaxHp; }
};
