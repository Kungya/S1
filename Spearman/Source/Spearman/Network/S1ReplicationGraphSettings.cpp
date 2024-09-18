// Fill out your copyright notice in the Description page of Project Settings.


#include "S1ReplicationGraphSettings.h"
#include "Misc/App.h"
#include "S1ReplicationGraph.h"

US1ReplicationGraphSettings::US1ReplicationGraphSettings()
{
	CategoryName = TEXT("Game");
	DefaultReplicationGraphClass = US1ReplicationGraph::StaticClass();
}
