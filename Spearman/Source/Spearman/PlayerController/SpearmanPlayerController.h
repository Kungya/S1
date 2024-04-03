// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SpearmanPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class SPEARMAN_API ASpearmanPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void SetHUDHp(float Hp, float MaxHp);
	virtual void OnPossess(APawn* InPawn) override;
protected:
	virtual void BeginPlay() override;

private:
	class ASpearmanHUD* SpearmanHUD;
};
