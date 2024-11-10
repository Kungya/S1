// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ReplicationGraph.h"
#include "S1ReplicationGraphTypes.h"
#include "S1ReplicationGraph.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogS1RepGraph, Display, All);

class ASpearmanCharacter;
class AWeapon;

/** S1 Replication Graph implementation. */
UCLASS(transient, config = Engine)
class US1ReplicationGraph : public UReplicationGraph
{
	GENERATED_BODY()
	
public:
	US1ReplicationGraph();

	virtual void ResetGameWorldState() override;

	virtual void InitGlobalActorClassSettings() override;
	virtual void InitGlobalGraphNodes() override;
	virtual void InitConnectionGraphNodes(UNetReplicationGraphConnection* RepGraphConnection) override;
	virtual void RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo) override;
	virtual void RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo) override;

	virtual bool ProcessRemoteFunction(class AActor* Actor, UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack, class UObject* SubObject) override;
	virtual int32 ServerReplicateActors(float DeltaSeconds) override;

	void AddVisibleActor(const FNewReplicatedActorInfo& ActorInfo);
	void RemoveVisibleActor(const FNewReplicatedActorInfo& ActorInfo);

	void SetDynamicSpatialFrequencyNodeRepPeriod();

	UPROPERTY()
	TObjectPtr<UReplicationGraphNode_GridSpatialization2D> GridNode;

	UPROPERTY()
	TObjectPtr<UReplicationGraphNode_ActorList> AlwaysRelevantNode;
	
	TMap<FName, FActorRepListRefView> AlwaysRelevantStreamingLevelActors;

	/** Actors that are replicated for specific connections, based on Visibility Check. */
	UPROPERTY()
	TMap<UNetConnection*, US1ReplicationGraphNode_VisibilityCheck_ForConnection*> VisibilityCheckForConnectionNodes;

	float CachedDeltaSeconds = 0.f;

	/** Actors that could "potentially" become visible or hidden */
	FActorRepListRefView PotentiallyVisibleActorList;

	/** BookKeep mutual visibility in VisibilityCheck_ForConnection, @See ProcessRemoteFunction()
	*** {Server -> Client} may not be kept. cuz Server has not NetConnection. **/
	TMap<TPair<AActor*, AActor*>, bool> VisibilityBookkeeping;

	/* For check performance */
	int32 LineTraceCounter = 0;

	void OnCharacterSwapWeapon(ASpearmanCharacter* Character, AWeapon* NewWeapon, AWeapon* OldWeapon);

	void PrintRepNodePolicies();

private:
	void AddClassRepInfo(UClass* Class, EClassRepNodeMapping Mapping);
	void RegisterClassRepNodeMapping(UClass* Class);
	EClassRepNodeMapping GetClassNodeMapping(UClass* Class) const;

	void RegisterClassReplicationInfo(UClass* ReplicatedClass);
	bool ConditionalInitClassReplicationInfo(UClass* ReplicatedClass, FClassReplicationInfo& ClassInfo);
	void InitClassReplicationInfo(FClassReplicationInfo& Info, UClass* Class, bool Spatialize) const;

	EClassRepNodeMapping GetMappingPolicy(UClass* Class);

	bool IsSpatialized(EClassRepNodeMapping Mapping) const { return Mapping >= EClassRepNodeMapping::Spatialize_Static; }
	TClassMap<EClassRepNodeMapping> ClassRepNodePolicies;

	TArray<UClass*> ExplicitlySetClasses;
};

struct FCachedAlwaysRelevantActorInfo
{
	TWeakObjectPtr<AActor> LastViewer;
	TWeakObjectPtr<AActor> LastViewTarget;
};

UCLASS()
class US1ReplicationGraphNode_AlwaysRelevant_ForConnection : public UReplicationGraphNode_AlwaysRelevant_ForConnection
{
	GENERATED_BODY()
	
public:
	virtual void NotifyAddNetworkActor(const FNewReplicatedActorInfo& Actor) override { }
	virtual bool NotifyRemoveNetworkActor(const FNewReplicatedActorInfo& ActorInfo, bool bWarnIfNotFound = true) override { return false; }
	virtual void NotifyResetAllNetworkActors() override { }

	virtual void GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params) override;

	void OnClientLevelVisibilityAdd(FName LevelName, UWorld* StreamingWorld);
	void OnClientLevelVisibilityRemove(FName LevelName);

	void ResetGameWorldState();

	/* @ Added in 5.2 */
	template<typename KeyType, typename ValueType>
	static void CleanupCachedRelevantActors(TMap<TObjectKey<KeyType>, ValueType>& ActorMap)
	{
		for (auto MapIt = ActorMap.CreateIterator(); MapIt; ++MapIt)
		{
			if (MapIt.Key().ResolveObjectPtr() == nullptr)
			{
				MapIt.RemoveCurrent();
			}
		}
	}
protected:
	/* @ Added in 5.2 */
	void UpdateCachedRelevantActor(const FConnectionGatherActorListParameters& Params, AActor* NewActor, TWeakObjectPtr<AActor>& LastActor);
	/* @ Added in 5.2, TArray PastRelevant is Deprecated. */
	TMap<TObjectKey<UNetConnection>, FCachedAlwaysRelevantActorInfo> PastRelevantActorMap;

private:
	TArray<FName, TInlineAllocator<64> > AlwaysRelevantStreamingLevelsNeedingReplication;

	bool bInitializePlayerState = false;
};

UCLASS()
class US1ReplicationGraphNode_VisibilityCheck_ForConnection : public UReplicationGraphNode_ActorList
{
	GENERATED_BODY()

public:
	US1ReplicationGraphNode_VisibilityCheck_ForConnection();
	
	virtual void PrepareForReplication() override;
	virtual void GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params) override;

	TWeakObjectPtr<UNetReplicationGraphConnection> ConnectionManager;
	TWeakObjectPtr<APawn> CachedPawn;

private:
	// Super::ReplicationList
};

UCLASS()
class US1ReplicationGraphNode_DynamicSpatialFrequency_VisibilityCheck : public UReplicationGraphNode_DynamicSpatialFrequency
{
	GENERATED_BODY()
	
public:
	US1ReplicationGraphNode_DynamicSpatialFrequency_VisibilityCheck();

	virtual void PrepareForReplication() override;
	virtual void GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params) override;

	TWeakObjectPtr<UNetReplicationGraphConnection> ConnectionManager;
	TWeakObjectPtr<APawn> CachedPawn;

private:

	bool CalcVisibilityForActor(AActor* ActorToCheck, const FGlobalActorReplicationInfo& GlobalInfoForActor, US1ReplicationGraph* S1RepGraph);

};