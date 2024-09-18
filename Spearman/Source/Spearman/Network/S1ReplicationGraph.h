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

	UPROPERTY()
	TObjectPtr<UReplicationGraphNode_GridSpatialization2D> GridNode;

	UPROPERTY()
	TObjectPtr<UReplicationGraphNode_ActorList> AlwaysRelevantNode;
	
	TMap<FName, FActorRepListRefView> AlwaysRelevantStreamingLevelActors;

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
	void UpdateCachedRelevantActor(const FConnectionGatherActorListParameters& Params, AActor* NewActor, TWeakObjectPtr<AActor>& LastActor);

	TMap<TObjectKey<UNetConnection>, FCachedAlwaysRelevantActorInfo> PastRelevantActorMap;

private:
	TArray<FName, TInlineAllocator<64> > AlwaysRelevantStreamingLevelsNeedingReplication;

	bool bInitializePlayerState = false;
};