// Fill out your copyright notice in the Description page of Project Settings.


#include "S1GameInstance.h"

US1GameInstance::US1GameInstance()
{
	
}

void US1GameInstance::Init()
{
	Super::Init();

	// Item ids for Client-side visual assets
	ItemIds = { 101, 102, 201, 202, 203 };
}

FItemData* US1GameInstance::GetItemData(int32 Id)
{
	return ItemDataTable->FindRow<FItemData>(*FString::FromInt(Id), TEXT(""));
}