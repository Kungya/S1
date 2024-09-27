// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreTypes.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "S1ReplicationGraphTypes.h"
#include "S1ReplicationGraphSettings.generated.h"

/**
 * Default settings for the S1 replication graph, Not using FastShared so far.
 */
UCLASS(config = Game, MinimalAPI)
class US1ReplicationGraphSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()
	
public:
	US1ReplicationGraphSettings();
	
public:

	UPROPERTY(config, EditAnywhere, Category = ReplicationGraph)
	bool bDisableReplicationGraph = true;

	UPROPERTY(config, EditAnywhere, Category = ReplicationGraph, meta = (MetaClass = "/Script/Spearman.S1ReplicationGraph"))
	FSoftClassPath DefaultReplicationGraphClass;

	/* TODO : FastShared Settings */

	UPROPERTY(EditAnywhere, Category = DestructionInfo, meta = (ForceUnits = cm, ConsoleVariable = "S1.RepGraph.DestructInfo.MaxDist"))
	float DestructionInfoMaxDist = 30000.f;

	UPROPERTY(EditAnywhere, Category = SpatialGrid, meta = (ForceUnits = cm, ConsoleVariable = "S1.RepGraph.CellSize"))
	float SpatialGridCellSize = 10000.0f;

	// Essentially "Min X" for replication. This is just an initial value. The system will reset itself if actors appears outside of this.
	UPROPERTY(EditAnywhere, Category = SpatialGrid, meta = (ForceUnits = cm, ConsoleVariable = "S1.RepGraph.SpatialBiasX"))
	float SpatialBiasX = -200000.0f;

	// Essentially "Min Y" for replication. This is just an initial value. The system will reset itself if actors appears outside of this.
	UPROPERTY(EditAnywhere, Category = SpatialGrid, meta = (ForceUnits = cm, ConsoleVariable = "S1.RepGraph.SpatialBiasY"))
	float SpatialBiasY = -200000.0f;

	UPROPERTY(EditAnywhere, Category = SpatialGrid, meta = (ConsoleVariable = "S1.RepGraph.DisableSpatialRebuilds"))
	bool bDisableSpatialRebuilds = true;

	// How many buckets to spread dynamic, spatialized actors across.
	// High number = more buckets = smaller effective replication frequency.
	// This happens before individual actors do their own NetUpdateFrequency check.
	UPROPERTY(EditAnywhere, Category = DynamicSpatialFrequency, meta = (ConsoleVariable = "S1.RepGraph.DynamicActorFrequencyBuckets"))
	int32 DynamicActorFrequencyBuckets = 3;

	/* S1ReplicationGraphTypes.h에 있는 값을 DefaultGame.ini에서 설정해주면서 여기서 들고 있게 되는 것*/
	// Array of Custom Settings for Specific Classes 
	UPROPERTY(config, EditAnywhere, Category = ReplicationGraph)
	TArray<FRepGraphActorClassSettings> ClassSettings;
};
