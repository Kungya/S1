// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMax")
};

UCLASS()
class SPEARMAN_API AWeapon : public AActor
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

	void AttackCollisionCheckByTrace();

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float Damage = 30.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float HeadShotDamage = 50.f;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	float FinalDamage;
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UBoxComponent* AttackCollisionBox;
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UBoxComponent* TraceStartBox;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UBoxComponent* TraceEndBox;
	
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;

	// crosshair texture (if equip weapon)
	
	UPROPERTY(EditAnywhere, Category = Crosshair)
	class UTexture2D* CrosshairCircle;

	UPROPERTY(EditAnywhere, Category = Crosshair)
	UTexture2D* CrosshairDot;

	bool bAttackCollisionTrace = false;

	UPROPERTY()
	TSet<AActor*> HitSet;

	ACharacter* OwnerCharacter;
	AController* OwnerController;
	
public:
	// 드랍되거나, 장착될 때마다 변경해줘야 함
	void SetWeaponState(EWeaponState State);
	FORCEINLINE void SetWeaponVisibility(bool bNewVisibility) const { WeaponMesh->SetVisibility(bNewVisibility); }
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE UTexture2D* GetCrosshairCircle() const { return CrosshairCircle; }
	FORCEINLINE UTexture2D* GetCrosshairDot() const { return CrosshairDot; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }
};