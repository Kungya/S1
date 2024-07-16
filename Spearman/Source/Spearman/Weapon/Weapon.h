// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RewindableActor.h"
#include "Weapon.generated.h"

class UBoxComponent;
class USphereComponent;
class UWidgetComponent;
class ASpearmanCharacter;
class ASpearmanPlayerController;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMax")
};

UCLASS()
class SPEARMAN_API AWeapon : public ARewindableActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void ShowPickupWidget(bool bShowPickupWidget);
	void Dropped();

	void TurnOnAttackCollision();
	void TurnOffAttackCollision();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHitEffect(AActor* HitActor, int32 InDamage, FVector_NetQuantize HitPoint, bool bHeadShot);

	UPROPERTY()
	TSet<AActor*> HitSet;

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
		);

	void AttackCollisionCheckByRewind();
	void AttackCollisionCheckByServer();

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float Damage = 30.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float HeadShotDamage = 50.f;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	float HitPartDamage;
	
	UPROPERTY(EditAnywhere)
	bool bUseRewind = false;

private:

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USphereComponent* AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UBoxComponent* AttackCollisionBox;
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UBoxComponent* TraceStartBox;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UBoxComponent* TraceEndBox;
	
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UWidgetComponent* PickupWidget;

	UPROPERTY(Replicated)
	bool bAttackCollisionTrace = false;

	UPROPERTY()
	ASpearmanCharacter* OwnerSpearmanCharacter;

	UPROPERTY()
	ASpearmanPlayerController* OwnerSpearmanPlayerController;
	
public:
	void SetWeaponState(EWeaponState State);
	void SetbAttackCollisionTrace() { bAttackCollisionTrace = false; }
	FORCEINLINE void CheckOwnerSpearmanCharacterIsValid() { OwnerSpearmanCharacter = (OwnerSpearmanCharacter == nullptr) ? Cast<ASpearmanCharacter>(GetOwner()) : OwnerSpearmanCharacter; }
	FORCEINLINE void SetWeaponVisibility(bool bNewVisibility) const { WeaponMesh->SetVisibility(bNewVisibility); }
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }
	FORCEINLINE UBoxComponent* GetAttackCollisionBox() const { return AttackCollisionBox; }
	FORCEINLINE ASpearmanCharacter* GetOwnerSpearmanCharacter() const { return OwnerSpearmanCharacter; }
};