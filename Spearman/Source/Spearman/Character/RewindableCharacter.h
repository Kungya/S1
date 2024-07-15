// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Spearman/Interfaces/RewindableInterface.h"
#include "RewindableCharacter.generated.h"

class UBoxComponent;
class UHistoryComponent;

UCLASS()
class SPEARMAN_API ARewindableCharacter : public ACharacter, public IRewindableInterface
{
	GENERATED_BODY()

public:
	ARewindableCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UBoxComponent*> HitBoxArray;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "ActorComponent")
	UHistoryComponent* History;

private:

	UPROPERTY(EditAnywhere, Category = Combat)
	FString HeadBone;


public:
	virtual TArray<UBoxComponent*>& GetHitBoxArray() override { return HitBoxArray; }
	virtual UHistoryComponent* GetHistory() override { return History; }
	FORCEINLINE FString GetHeadBone() const { return HeadBone; }	
};