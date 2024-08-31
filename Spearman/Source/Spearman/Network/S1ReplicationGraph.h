// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ReplicationGraph.h"
#include "S1ReplicationGraph.generated.h"

UCLASS()
class UReplicationGraphNode_AlwaysRelevant_WithPending : public UReplicationGraphNode_ActorList
{
	GENERATED_BODY()
	
public:
	UReplicationGraphNode_AlwaysRelevant_WithPending();
	virtual void PrepareForReplication() override;
};

UCLASS()
class UReplicationGraphNode_AlwaysRelevant_ForTeam : public UReplicationGraphNode_ActorList
{
	GENERATED_BODY()
	
	virtual void GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params) override;
	virtual void GatherActorListsForConnectionDefault(const FConnectionGatherActorListParameters& Params);
};

UCLASS()
class US1ConnectionManager : public UNetReplicationGraphConnection
{
	GENERATED_BODY()

public:
	
	UPROPERTY()
	UReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantForConnectionNode;

	UPROPERTY()
	UReplicationGraphNode_AlwaysRelevant_ForTeam* TeamConnectionNode;

	int32 Team = -1;

	UPROPERTY()
	TWeakObjectPtr<APawn> Pawn = nullptr;
};

struct FTeamConnectionListMap : TMap<int32, TArray<US1ConnectionManager*>>
{
	TArray<US1ConnectionManager*>* GetConnectionArrayForTeam(int32 Team);
	TArray<US1ConnectionManager*> GetVisibleConnectionArrayForNonTeam(const APawn* Pawn, int32 Team);

	void AddConnectionToTeam(int32 Team, US1ConnectionManager* ConnectionManager);
	void RemoveConnectionFromTeam(int32 Team, US1ConnectionManager* ConnectionManager);
};

UCLASS(Blueprintable)
class SPEARMAN_API US1ReplicationGraph : public UReplicationGraph
{
	GENERATED_BODY()
	
	friend UReplicationGraphNode_AlwaysRelevant_ForTeam;

public:
	US1ReplicationGraph();

	virtual void InitGlobalGraphNodes() override;
	virtual void InitConnectionGraphNodes(UNetReplicationGraphConnection* ConnetionManager) override;

	virtual void RemoveClientConnection(UNetConnection* NetConnection) override;
	virtual void ResetGameWorldState() override;

	virtual void RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo) override;
	virtual void RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo) override;

	void SetTeamForPlayerController(APlayerController* PlayerController, int32 Team);
	void HandlePendingActorAndTeamReqeusts();

private:
	UPROPERTY()
	UReplicationGraphNode_AlwaysRelevant_WithPending* AlwaysRelevantNode;

	UPROPERTY()
	TArray<AActor*> PendingConnectionActors;
	
	UPROPERTY()
	TArray<TTuple<int32, APlayerController*>> PendingTeamReqeusts;

	FTeamConnectionListMap TeamConnectionListMap;

	US1ConnectionManager* GetS1ConnectionManagerFromActor(const AActor* Actor);
};
