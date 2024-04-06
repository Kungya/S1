// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicMonsterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Spearman/Monster/BasicMonster.h"
#include "BehaviorTree/BehaviorTree.h"

ABasicMonsterAIController::ABasicMonsterAIController()
{
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
	check(BlackboardComponent);

	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	check(BehaviorTreeComponent);
}

void ABasicMonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn == nullptr) return;

	ABasicMonster* BasicMonster = Cast<ABasicMonster>(InPawn);
	if (BasicMonster)
	{
		if (BasicMonster->GetBehaviorTree())
		{
			BlackboardComponent->InitializeBlackboard(*(BasicMonster->GetBehaviorTree()->BlackboardAsset));
		}
	}
}
