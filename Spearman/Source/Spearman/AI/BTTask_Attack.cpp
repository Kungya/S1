// Fill out your copyright notice in the Description page of Project Settings.

#include "BTTask_Attack.h"
#include "BasicMonsterAIController.h"
#include "Spearman/Monster/BasicMonster.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = "Attack";
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	if (OwnerComp.GetAIOwner() == nullptr) return EBTNodeResult::Failed;

	ABasicMonster* BasicMonster = Cast<ABasicMonster>(OwnerComp.GetAIOwner()->GetPawn());

	if (BasicMonster && BasicMonster->HasAuthority())
	{
		BasicMonster->MulticastAttack();
		BasicMonster->SetCanAttackTimer();

		return EBTNodeResult::Succeeded;
	}

	// TODO : Client side return failed?
	return EBTNodeResult::Failed;
}
