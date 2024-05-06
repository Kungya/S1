// Fill out your copyright notice in the Description page of Project Settings.


#include "S1GameInstance.h"

US1GameInstance::US1GameInstance()
{
	
}

void US1GameInstance::Init()
{
	Super::Init();

	UE_LOG(LogTemp, Warning, TEXT("GameInstancem %d"), GetItemData(102)->Weight);
}

FItemData* US1GameInstance::GetItemData(int32 Id)
{
	// ContextStr : 세밀하게 찾을 때
	return ItemDataTable->FindRow<FItemData>(*FString::FromInt(Id), TEXT(""));
}