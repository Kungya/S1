// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spearman/Interfaces/RewindableInterface.h"
#include "RewindableActor.generated.h"

class UBoxComponent;
class UHistoryComponent;

UCLASS()
class SPEARMAN_API ARewindableActor : public AActor, public IRewindableInterface
{
	GENERATED_BODY()
	
public:	
	ARewindableActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UBoxComponent*> HitBoxArray;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "ActorComponent")
	UHistoryComponent* History;

private:

public:
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	virtual TArray<UBoxComponent*>& GetHitBoxArray() override { return HitBoxArray; }
	virtual UHistoryComponent* GetHistory() override { return History; }

};
