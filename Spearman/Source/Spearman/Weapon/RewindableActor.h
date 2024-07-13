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

	virtual TArray<UBoxComponent*>& GetHitBoxArray() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UBoxComponent*> HitBoxArray;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "ActorComponent")
	UHistoryComponent* History;

private:

public:	
	FORCEINLINE UHistoryComponent* GetHistory() const { return History; }

};
