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

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnAttacked(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UParticleSystem* HitParticles;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float MaxHp = 75.f;

	UPROPERTY(ReplicatedUsing = OnRep_Hp, VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float Hp = 75.f;

	UFUNCTION()
	void OnRep_Hp();


public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void WeaponHit_Implementation(FHitResult HitResult) override;
};
