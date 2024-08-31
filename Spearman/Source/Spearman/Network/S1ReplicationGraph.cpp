// Fill out your copyright notice in the Description page of Project Settings.


#include "S1ReplicationGraph.h"


UReplicationGraphNode_AlwaysRelevant_WithPending::UReplicationGraphNode_AlwaysRelevant_WithPending()
{
	// Call PrepareForReplication before replication once per frame
	bRequiresPrepareForReplicationCall = true;
}

void UReplicationGraphNode_AlwaysRelevant_WithPending::PrepareForReplication()
{
	US1ReplicationGraph* S1ReplicationGraph = Cast<US1ReplicationGraph>(GetOuter());
	S1ReplicationGraph->HandlePendingActorsAndTeamRequests();
}

US1ReplicationGraph::US1ReplicationGraph()
{
	// Specify the connection graph class to use
	ReplicationConnectionManagerClass = US1ConnectionManager::StaticClass();
}

void UReplicationGraphNode_AlwaysRelevant_ForTeam::GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params)
{

}

void UReplicationGraphNode_AlwaysRelevant_ForTeam::GatherActorListsForConnectionDefault(const FConnectionGatherActorListParameters& Params)
{
	Super::GatherActorListsForConnection(Params);
}

/* ... many things */

void US1ReplicationGraph::InitGlobalGraphNodes()
{ // AlwayRelevant : 항상 복제받는 규칙 (가장 간단)
	Super::InitGlobalGraphNodes();

	// Create the always relevant node
	AlwaysRelevantNode = CreateNewNode<UReplicationGraphNode_AlwaysRelevant_WithPending>();
	AddGlobalGraphNode(AlwaysRelevantNode);
}

void US1ReplicationGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection* ConnectionManager)
{ // Player별 규칙 ()
	Super::InitConnectionGraphNodes(ConnectionManager);

	// Create
	US1ConnectionManager* S1ConnectionManager = Cast<US1ConnectionManager>(ConnectionManager);

	if (ensure(S1ConnectionManager))
	{
		S1ConnectionManager->AlwaysRelevantForConnectionNode = CreateNewNode<UReplicationGraphNode_AlwaysRelevant_ForConnection>();
		AddConnectionGraphNode(S1ConnectionManager->AlwaysRelevantForConnectionNode, ConnectionManager);

		S1ConnectionManager->TeamConnectionNode = CreateNewNode<UReplicationGraphNode_AlwaysRelevant_ForTeam>();
		AddConnectionGraphNode(S1ConnectionManager->TeamConnectionNode, ConnectionManager);
	}
}

