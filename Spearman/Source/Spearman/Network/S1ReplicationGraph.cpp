// Fill out your copyright notice in the Description page of Project Settings.


#include "S1ReplicationGraph.h"

#include "Net/UnrealNetwork.h"
#include "Engine/LevelStreaming.h"
#include "EngineUtils.h"
#include "CoreGlobals.h"

#include "Net/RepLayout.h"
#include "Net/Core/Trace/NetTrace.h"
#include "Engine/ChildConnection.h"
#include "HAL/IConsoleManager.h"

#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/NetConnection.h"
#include "UObject/UObjectIterator.h"

#include "S1ReplicationGraphSettings.h"
#include "Spearman/Character/SpearmanCharacter.h"
#include "Spearman/PlayerController/SpearmanPlayerController.h"
#include "Spearman/Weapon/Weapon.h"
#include "Spearman/Spearman.h"
#include "Spearman/SpearComponents/CombatComponent.h"

#include "DrawDebugHelpers.h"


DEFINE_LOG_CATEGORY(LogS1RepGraph);

namespace S1::RepGraph
{
	float DestructionInfoMaxDist = 30000.f;
	static FAutoConsoleVariableRef CvarS1RepGraphDestructMaxDist(TEXT("S1.RepGraph.DestructInfo.MaxDist"), DestructionInfoMaxDist, TEXT("Max distance (not squared) to rep destruct infos at"), ECVF_Default);

	int32 DisplayClientLevelStreaming = 1;
	static FAutoConsoleVariableRef CVarS1RepGraphDisplayClientLevelStreaming(TEXT("S1.RepGraph.DisplayClientLevelStreaming"), DisplayClientLevelStreaming, TEXT(""), ECVF_Default);

	float CellSize = 10000.f;
	static FAutoConsoleVariableRef CVarS1RepGraphCellSize(TEXT("S1.RepGraph.CellSize"), CellSize, TEXT(""), ECVF_Default);

	// Essentially "Min X" for replication. This is just an initial value. The system will reset itself if actors appears outside of this.
	float SpatialBiasX = -150000.f;
	static FAutoConsoleVariableRef CVarS1RepGraphSpatialBiasX(TEXT("S1.RepGraph.SpatialBiasX"), SpatialBiasX, TEXT(""), ECVF_Default);

	// Essentially "Min Y" for replication. This is just an initial value. The system will reset itself if actors appears outside of this.
	float SpatialBiasY = -200000.f;
	static FAutoConsoleVariableRef CVarS1RepSpatialBiasY(TEXT("S1.RepGraph.SpatialBiasY"), SpatialBiasY, TEXT(""), ECVF_Default);

	// How many buckets to spread dynamic, spatialized actors across. High number = more buckets = smaller effective replication frequency. This happens before individual actors do their own NetUpdateFrequency check.
	int32 DynamicActorFrequencyBuckets = 3;
	static FAutoConsoleVariableRef CVarS1RepDynamicActorFrequencyBuckets(TEXT("S1.RepGraph.DynamicActorFrequencyBuckets"), DynamicActorFrequencyBuckets, TEXT(""), ECVF_Default);

	int32 DisableSpatialRebuilds = 1;
	static FAutoConsoleVariableRef CVarS1RepDisableSpatialRebuilds(TEXT("S1.RepGraph.DisableSpatialRebuilds"), DisableSpatialRebuilds, TEXT(""), ECVF_Default);

	int32 LogLazyInitClasses = 0;
	static FAutoConsoleVariableRef CVarS1RepLogLazyInitClasses(TEXT("S1.RepGraph.LogLazyInitClasses"), LogLazyInitClasses, TEXT(""), ECVF_Default);

	int32 EnableFastSharedPath = 0;
	static FAutoConsoleVariableRef CVarS1RepEnableFastSharedPath(TEXT("S1.RepGraph.EnableFastSharedPath"), EnableFastSharedPath, TEXT(""), ECVF_Default);

	UReplicationDriver* ConditionalCreateReplicationDriver(UNetDriver* ForNetDriver, UWorld* World)
	{
		// Only create for GameNetDriver
		if (World && ForNetDriver && ForNetDriver->NetDriverName == NAME_GameNetDriver)
		{
			const US1ReplicationGraphSettings* S1RepGraphSettings = GetDefault<US1ReplicationGraphSettings>();

			// Enable/Disable via developer settings
			if (S1RepGraphSettings && S1RepGraphSettings->bDisableReplicationGraph)
			{
				return nullptr;
			}

			TSubclassOf<US1ReplicationGraph> GraphClass = S1RepGraphSettings->DefaultReplicationGraphClass.TryLoadClass<US1ReplicationGraph>();
			if (GraphClass.Get() == nullptr)
			{
				GraphClass = US1ReplicationGraph::StaticClass();
			}

			US1ReplicationGraph* S1ReplicationGraph = NewObject<US1ReplicationGraph>(GetTransientPackage(), GraphClass.Get());
			return S1ReplicationGraph;
		}

		return nullptr;
	}
}

/* ---------------------------------------- From ReplicationGraph.cpp Start ---------------------------------------- */
REPGRAPH_DEVCVAR_SHIPCONST(int32, "Net.RepGraph.EnableRPCSendPolicy", CVar_RepGraph_EnableRPCSendPolicy, 1, "Enables RPC send policy (e.g, force certain functions to send immediately rather than be queued)");

static TAutoConsoleVariable<FString> CVarRepGraphConditionalBreakpointActorName(TEXT("Net.RepGraph.ConditionalBreakpointActorName"), TEXT(""),
	TEXT("Helper CVar for debugging. Set this string to conditionally log/breakpoint various points in the repgraph pipeline. Useful for bugs like 'why is this actor channel closing'"), ECVF_Default);

FActorConnectionPair DebugActorConnectionPair;

FORCEINLINE bool RepGraphConditionalActorBreakpoint(AActor* Actor, UNetConnection* NetConnection)
{
#if !(UE_BUILD_SHIPPING)
	if (CVarRepGraphConditionalBreakpointActorName.GetValueOnGameThread().Len() > 0 && GetNameSafe(Actor).Contains(CVarRepGraphConditionalBreakpointActorName.GetValueOnGameThread()))
	{
		return true;
	}

	// Alternatively, DebugActorConnectionPair can be set by code to catch a specific actor/connection pair 
	if (DebugActorConnectionPair.Actor.Get() == Actor && (DebugActorConnectionPair.Connection == nullptr || DebugActorConnectionPair.Connection == NetConnection))
	{
		return true;
	}
#endif
	return false;
}

FORCEINLINE bool ReadyForNextReplication(FConnectionReplicationActorInfo& ConnectionData, FGlobalActorReplicationInfo& GlobalData, const uint32 FrameNum)
{
	return (ConnectionData.NextReplicationFrameNum <= FrameNum || GlobalData.ForceNetUpdateFrame > ConnectionData.LastRepFrameNum);
}

FORCEINLINE bool ReadyForNextReplication_FastPath(FConnectionReplicationActorInfo& ConnectionData, FGlobalActorReplicationInfo& GlobalData, const uint32 FrameNum)
{
	return (ConnectionData.FastPath_NextReplicationFrameNum <= FrameNum || GlobalData.ForceNetUpdateFrame > ConnectionData.FastPath_LastRepFrameNum);
}

/* ---------------------------------------- From ReplicationGraph.cpp End ---------------------------------------- */

/* ---------------------------------------- S1ReplicationGraph Code Segment Start ---------------------------------------- */

US1ReplicationGraph::US1ReplicationGraph()
{
	if (!UReplicationDriver::CreateReplicationDriverDelegate().IsBound())
	{
		UReplicationDriver::CreateReplicationDriverDelegate().BindLambda(
			[](UNetDriver* ForNetDriver, const FURL& URL, UWorld* World) -> UReplicationDriver*
			{
				return S1::RepGraph::ConditionalCreateReplicationDriver(ForNetDriver, World);
			});
	}
}

void US1ReplicationGraph::ResetGameWorldState()
{
	Super::ResetGameWorldState();

	AlwaysRelevantStreamingLevelActors.Empty();

	for (UNetReplicationGraphConnection* ConnectionManager : Connections)
	{
		for (UReplicationGraphNode* ConnectionNode : ConnectionManager->GetConnectionGraphNodes())
		{
			if (US1ReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = Cast<US1ReplicationGraphNode_AlwaysRelevant_ForConnection>(ConnectionNode))
			{
				AlwaysRelevantConnectionNode->ResetGameWorldState();
			}
		}
	}

	for (UNetReplicationGraphConnection* ConnectionManager : PendingConnections)
	{
		for (UReplicationGraphNode* ConnectionNode : ConnectionManager->GetConnectionGraphNodes())
		{
			if (US1ReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = Cast<US1ReplicationGraphNode_AlwaysRelevant_ForConnection>(ConnectionNode))
			{
				AlwaysRelevantConnectionNode->ResetGameWorldState();
			}
		}
	}

	UE_LOG(LogS1RepGraph, Warning, TEXT("Called [ResetGameWorldState] !"));
}

EClassRepNodeMapping US1ReplicationGraph::GetClassNodeMapping(UClass* Class) const
{
	if (!Class)
	{
		return EClassRepNodeMapping::NotRouted;
	}

	if (const EClassRepNodeMapping* Ptr = ClassRepNodePolicies.FindWithoutClassRecursion(Class))
	{
		return *Ptr;
	}

	if (Class->IsChildOf(ASpearmanCharacter::StaticClass()))
	{
		UE_LOG(LogS1RepGraph, Error, TEXT("SpearmanCharacter's Policy is VisibilityChecked"));
		return EClassRepNodeMapping::VisibilityChecked;
	}

	AActor* ActorCDO = Cast<AActor>(Class->GetDefaultObject());
	if (!ActorCDO || !ActorCDO->GetIsReplicated())
	{
		return EClassRepNodeMapping::NotRouted;
	}
	
	auto ShouldSpatialize = [](const AActor* CDO)
	{ // Spatialize되지 않을려면 후행하는 3개의 조건 중에서 하나라도 충족해야 함.
		return CDO->GetIsReplicated() && (!(CDO->bAlwaysRelevant || CDO->bOnlyRelevantToOwner || CDO->bNetUseOwnerRelevancy));
	};

	UClass* SuperClass = Class->GetSuperClass();
	if (AActor* SuperCDO = Cast<AActor>(SuperClass->GetDefaultObject()))
	{ // bAlwaysRelevant bOnlyRelevantToOwner, bNetUseOwnerRelevancy
		if (SuperCDO->GetIsReplicated() == ActorCDO->GetIsReplicated()
			&& SuperCDO->bAlwaysRelevant == ActorCDO->bAlwaysRelevant
			&& SuperCDO->bOnlyRelevantToOwner == ActorCDO->bOnlyRelevantToOwner
			&& SuperCDO->bNetUseOwnerRelevancy == ActorCDO->bNetUseOwnerRelevancy
			)
		{
			return GetClassNodeMapping(SuperClass);
		}
	}

	if (ShouldSpatialize(ActorCDO))
	{
		return EClassRepNodeMapping::Spatialize_Dynamic;
	}
	else if (ActorCDO->bAlwaysRelevant && !ActorCDO->bOnlyRelevantToOwner)
	{
		return EClassRepNodeMapping::RelevantAllConnections;
	}

	return EClassRepNodeMapping::NotRouted;
}

void US1ReplicationGraph::RegisterClassRepNodeMapping(UClass* Class)
{
	EClassRepNodeMapping Mapping = GetClassNodeMapping(Class);
	ClassRepNodePolicies.Set(Class, Mapping);
}

void US1ReplicationGraph::InitClassReplicationInfo(FClassReplicationInfo& Info, UClass* Class, bool Spatialize) const
{
	AActor* CDO = Class->GetDefaultObject<AActor>();
	if (Spatialize)
	{
		Info.SetCullDistanceSquared(CDO->NetCullDistanceSquared);
	}

	Info.ReplicationPeriodFrame = GetReplicationPeriodFrameForFrequency(CDO->NetUpdateFrequency);

	UClass* NativeClass = Class;
	while (!NativeClass->IsNative() && NativeClass->GetSuperClass() && NativeClass->GetSuperClass() != AActor::StaticClass())
	{
		NativeClass = NativeClass->GetSuperClass();
	}

	UE_LOG(LogS1RepGraph, Log, TEXT("Setting replication period for %s (%s) to %d frames (%.2f)"), *Class->GetName(), *NativeClass->GetName(), Info.ReplicationPeriodFrame, CDO->NetUpdateFrequency);
}

bool US1ReplicationGraph::ConditionalInitClassReplicationInfo(UClass* ReplicatedClass, FClassReplicationInfo& ClassInfo)
{
	if (ExplicitlySetClasses.FindByPredicate([&](const UClass* SetClass) { return ReplicatedClass->IsChildOf(SetClass); }) != nullptr)
	{
		return false;
	}

	bool ClassIsSpatialized = IsSpatialized(ClassRepNodePolicies.GetChecked(ReplicatedClass));
	InitClassReplicationInfo(ClassInfo, ReplicatedClass, ClassIsSpatialized);
	return true;
}

void US1ReplicationGraph::AddClassRepInfo(UClass* Class, EClassRepNodeMapping Mapping)
{
	if (IsSpatialized(Mapping))
	{
		if (Class->GetDefaultObject<AActor>()->bAlwaysRelevant)
		{
			UE_LOG(LogS1RepGraph, Warning, TEXT("Replicated Class %s is AlwaysRelevant but is initialized into a spatialized node (%s)"), *Class->GetName(), *StaticEnum<EClassRepNodeMapping>()->GetNameStringByValue((int64)Mapping));
		}
	}

	ClassRepNodePolicies.Set(Class, Mapping);
}

void US1ReplicationGraph::RegisterClassReplicationInfo(UClass* ReplicatedClass)
{ // Add to "Global"ActorReplicationInfoMap
	FClassReplicationInfo ClassInfo;;
	if (ConditionalInitClassReplicationInfo(ReplicatedClass, ClassInfo))
	{
		GlobalActorReplicationInfoMap.SetClassInfo(ReplicatedClass, ClassInfo);
		UE_LOG(LogS1RepGraph, Log, TEXT("Setting %s - %.2f"), *GetNameSafe(ReplicatedClass), ClassInfo.GetCullDistance());
	}
}

void US1ReplicationGraph::InitGlobalActorClassSettings()
{
	/* 아래 두개의 lambda는 Map의 InitNewElement Functor를 지정해주는건데, 이 Functor는 Get()에서 사용됨. */
	// Setup our lazy init function for classes that are not currently loaded.
	GlobalActorReplicationInfoMap.SetInitClassInfoFunc(
		[this](UClass* Class, FClassReplicationInfo& ClassInfo)
		{
			RegisterClassRepNodeMapping(Class);

			const bool bHandled = ConditionalInitClassReplicationInfo(Class, ClassInfo);

			return bHandled;
		}
	);

	ClassRepNodePolicies.InitNewElement = [this](UClass* Class, EClassRepNodeMapping& NodeMapping)
	{
		NodeMapping = GetClassNodeMapping(Class);
		return true;
	};

	const US1ReplicationGraphSettings* S1RepGraphSettings = GetDefault<US1ReplicationGraphSettings>();
	check(S1RepGraphSettings);

	for (const FRepGraphActorClassSettings& ActorClassSettings : S1RepGraphSettings->ClassSettings)
	{ /* config에서 설정된 사전정보를 긁어서 Class에 대한 정책을 수동 적용하는 것. */
		if (ActorClassSettings.bAddClassRepInfoToMap)
		{
			if (UClass* StaticActorClass = ActorClassSettings.GetStaticActorClass())
			{
				UE_LOG(LogS1RepGraph, Log, TEXT("ActorClassSettings -- AddClassRepInfo - %s :: %i"), *StaticActorClass->GetName(), ActorClassSettings.ClassNodeMapping);
				AddClassRepInfo(StaticActorClass, ActorClassSettings.ClassNodeMapping);
			}
		}
	}

	TArray<UClass*> AllReplicatedClasses;

	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		AActor* ActorCDO = Cast<AActor>(Class->GetDefaultObject());
		if (!ActorCDO || !ActorCDO->GetIsReplicated())
		{
			continue;
		}

		if (Class->GetName().StartsWith(TEXT("SKEL_")) || Class->GetName().StartsWith(TEXT("REINST_")))
		{
			continue;
		}

		AllReplicatedClasses.Add(Class);

		RegisterClassRepNodeMapping(Class);
	}

	/* 여기 바로 아래에 있는 것들은 explicitly하게 설정하는 것이고, 아래 AllReplicatedClass를 for loop를 돌며 RegisterClassReplicationInf()하는건 Legacy에 해당. */
	auto SetClassInfo = [&](UClass* Class, const FClassReplicationInfo& Info)
	{
		GlobalActorReplicationInfoMap.SetClassInfo(Class, Info);
		ExplicitlySetClasses.Add(Class);
	};

	ExplicitlySetClasses.Reset();

	FClassReplicationInfo CharacterClassRepInfo;
	CharacterClassRepInfo.DistancePriorityScale = 1.f;
	CharacterClassRepInfo.StarvationPriorityScale = 1.f;
	CharacterClassRepInfo.ActorChannelFrameTimeout = 4;
	CharacterClassRepInfo.SetCullDistanceSquared(ASpearmanCharacter::StaticClass()->GetDefaultObject<ASpearmanCharacter>()->NetCullDistanceSquared);
	
	/* ACharacter와 그걸 상속받는 클래스들은 전부 위의 ReplicationInfo를 가지겠습니다 라고 명시적으로 설정.. */
	/* TODO : BasicMonster */
	SetClassInfo(ASpearmanCharacter::StaticClass(), CharacterClassRepInfo);
	
	UReplicationGraphNode_ActorListFrequencyBuckets::DefaultSettings.ListSize = 12;
	/* TODO : DefaultSettings if you use FastShared */

	RPCSendPolicyMap.Reset();

	/* Legacy set, skip if already Init info, via ConditionalInitClassReplicationInfo() */
	for (UClass* ReplicatedClass : AllReplicatedClasses)
	{
		RegisterClassReplicationInfo(ReplicatedClass);
	}
	
	DestructInfoMaxDistanceSquared = S1::RepGraph::DestructionInfoMaxDist * S1::RepGraph::DestructionInfoMaxDist;

	RPC_Multicast_OpenChannelForClass.Reset();
	RPC_Multicast_OpenChannelForClass.Set(AActor::StaticClass(), true); // Open channels for multicast RPCs by default
	RPC_Multicast_OpenChannelForClass.Set(AController::StaticClass(), false); // Multicasts should never open channels on Controllers since opening a channel on a non-owner breaks the Controller's replication.
	RPC_Multicast_OpenChannelForClass.Set(AServerStatReplicator::StaticClass(), false);
	
	for (const FRepGraphActorClassSettings& ActorClassSettings : S1RepGraphSettings->ClassSettings)
	{
		if (ActorClassSettings.bAddToRPC_Multicast_OpenChannelForClassMap)
		{
			if (UClass* StaticActorClass = ActorClassSettings.GetStaticActorClass())
			{
				UE_LOG(LogS1RepGraph, Log, TEXT("ActorClassSettings -- RPC_Multicast_OpenChannelForClass - %s"), *StaticActorClass->GetName());
				RPC_Multicast_OpenChannelForClass.Set(StaticActorClass, ActorClassSettings.bRPC_Multicast_OpenChannelForClass);
			}
		}
	}

	ASpearmanCharacter::NotifySwapWeapon.AddUObject(this, &US1ReplicationGraph::OnCharacterSwapWeapon);
	
	/* @See DynamicSpatialFrequency_VisibilityCheck */
	SetDynamicSpatialFrequencyNodeRepPeriod();

	UE_LOG(LogS1RepGraph, Warning, TEXT("Called [InitGlobalActorClassSettings] !"));
}

void US1ReplicationGraph::InitGlobalGraphNodes()
{
	// Spatial Actors
	GridNode = CreateNewNode<UReplicationGraphNode_GridSpatialization2D>();
	GridNode->CellSize = S1::RepGraph::CellSize;
	GridNode->SpatialBias = FVector2D(S1::RepGraph::SpatialBiasX, S1::RepGraph::SpatialBiasY);

	if (S1::RepGraph::DisableSpatialRebuilds)
	{
		GridNode->AddToClassRebuildDenyList(AActor::StaticClass());
	}

	AddGlobalGraphNode(GridNode);

	// Always Relevant Actors
	AlwaysRelevantNode = CreateNewNode<UReplicationGraphNode_ActorList>();
	AddGlobalGraphNode(AlwaysRelevantNode);

	// TODO : PlayerState Frequency Limiter

	UE_LOG(LogS1RepGraph, Warning, TEXT("Called [InitGlobalGraphNodes] !"));
}

void US1ReplicationGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection* RepGraphConnection)
{
	Super::InitConnectionGraphNodes(RepGraphConnection);

	/* [ AlwaysRelevant_ForConnection ] Node */
	US1ReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = CreateNewNode<US1ReplicationGraphNode_AlwaysRelevant_ForConnection>();
	
	RepGraphConnection->OnClientVisibleLevelNameAdd.AddUObject(AlwaysRelevantConnectionNode, &US1ReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityAdd);
	RepGraphConnection->OnClientVisibleLevelNameRemove.AddUObject(AlwaysRelevantConnectionNode, &US1ReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityRemove);

	AddConnectionGraphNode(AlwaysRelevantConnectionNode, RepGraphConnection);

	/* [ VisibilityCheck_ForConnection ] Node */
	/*US1ReplicationGraphNode_VisibilityCheck_ForConnection* VisibilityCheckConnectionNode = CreateNewNode<US1ReplicationGraphNode_VisibilityCheck_ForConnection>();

	AddConnectionGraphNode(VisibilityCheckConnectionNode, RepGraphConnection);

	VisibilityCheckForConnectionNodes.Add(RepGraphConnection->NetConnection, VisibilityCheckConnectionNode);
	VisibilityCheckConnectionNode->ConnectionManager = RepGraphConnection;*/

	/* [DynamicSpatialFrequency_VisibilityCheck] Node, Notes : Use Only either VisibilityCheck_ForConnection or this. */
	US1ReplicationGraphNode_DynamicSpatialFrequency_VisibilityCheck* DynamicSpatialFrequencyVisibilityCheckConnectionNode = CreateNewNode<US1ReplicationGraphNode_DynamicSpatialFrequency_VisibilityCheck>();

	AddConnectionGraphNode(DynamicSpatialFrequencyVisibilityCheckConnectionNode, RepGraphConnection);

	DynamicSpatialFrequencyVisibilityCheckConnectionNode->ConnectionManager = RepGraphConnection;


	UE_LOG(LogS1RepGraph, Warning, TEXT("Called [InitConnectionGraphNodes], VisibilityCheckForConnectionNodes.Num() : %d"), VisibilityCheckForConnectionNodes.Num());
}

EClassRepNodeMapping US1ReplicationGraph::GetMappingPolicy(UClass* Class)
{
	EClassRepNodeMapping* PolicyPtr = ClassRepNodePolicies.Get(Class);
	EClassRepNodeMapping Policy = PolicyPtr ? *PolicyPtr : EClassRepNodeMapping::NotRouted;
	return Policy;
}

void US1ReplicationGraph::AddVisibleActor(const FNewReplicatedActorInfo& ActorInfo)
{
	PotentiallyVisibleActorList.Add(ActorInfo.Actor);
}

void US1ReplicationGraph::RemoveVisibleActor(const FNewReplicatedActorInfo& ActorInfo)
{
	PotentiallyVisibleActorList.RemoveFast(ActorInfo.Actor);
}

/** Don't change these indices. @See DefaultSpatializationZones_NoFastShared() in ReplicationGraph.cpp */
void US1ReplicationGraph::SetDynamicSpatialFrequencyNodeRepPeriod()
{
	const int32 Rear = 0;
	const int32 Side = 1;
	const int32 Front = 2;

	UReplicationGraphNode_DynamicSpatialFrequency::DefaultSettings.ZoneSettings_NonFastSharedActors[Rear].MinRepPeriod = 7;
	UReplicationGraphNode_DynamicSpatialFrequency::DefaultSettings.ZoneSettings_NonFastSharedActors[Rear].MaxRepPeriod = 8;
	UReplicationGraphNode_DynamicSpatialFrequency::DefaultSettings.ZoneSettings_NonFastSharedActors[Side].MinRepPeriod = 5;
	UReplicationGraphNode_DynamicSpatialFrequency::DefaultSettings.ZoneSettings_NonFastSharedActors[Side].MaxRepPeriod = 8;
	UReplicationGraphNode_DynamicSpatialFrequency::DefaultSettings.ZoneSettings_NonFastSharedActors[Front].MinRepPeriod = 3;
	UReplicationGraphNode_DynamicSpatialFrequency::DefaultSettings.ZoneSettings_NonFastSharedActors[Front].MaxRepPeriod = 8;
}

void US1ReplicationGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo)
{
	EClassRepNodeMapping Policy = GetMappingPolicy(ActorInfo.Class);
	
 	switch (Policy)
	{
		case EClassRepNodeMapping::NotRouted:
		{ /* i.e : AWeapon, APlayerController */
			break;
		}

		case EClassRepNodeMapping::VisibilityChecked:
		{ /* i.e. : SpearmanCharacter */
			AddVisibleActor(ActorInfo);
			break;
		}

		case EClassRepNodeMapping::RelevantAllConnections:
		{
			if (ActorInfo.StreamingLevelName == NAME_None)
			{
				AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
			}
			else
			{
				FActorRepListRefView& RepList = AlwaysRelevantStreamingLevelActors.FindOrAdd(ActorInfo.StreamingLevelName);
				RepList.ConditionalAdd(ActorInfo.Actor);
			}

			break;
		}

		case EClassRepNodeMapping::Spatialize_Static:
		{
			GridNode->AddActor_Static(ActorInfo, GlobalInfo);
			break;
		}

		case EClassRepNodeMapping::Spatialize_Dynamic:
		{
			GridNode->AddActor_Dynamic(ActorInfo, GlobalInfo);
			break;
		}

		case EClassRepNodeMapping::Sptatialize_Dormancy:
		{
			GridNode->AddActor_Dormancy(ActorInfo, GlobalInfo);
			break;
		}
	};

	UE_LOG(LogS1RepGraph, Warning, TEXT("[RouteAddNetworkActorToNodes] : {C, P} => {%d, %d}"), Connections.Num(), PendingConnections.Num());
}

void US1ReplicationGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo)
{
	EClassRepNodeMapping Policy = GetMappingPolicy(ActorInfo.Class);

	switch (Policy)
	{
		case EClassRepNodeMapping::NotRouted:
		{
			break;
		}

		case EClassRepNodeMapping::VisibilityChecked:
		{ // not routed, just add actor in PotentiallyVisibleActorList.
			RemoveVisibleActor(ActorInfo);
			break;
		}
		
		case EClassRepNodeMapping::RelevantAllConnections:
		{
			if (ActorInfo.StreamingLevelName == NAME_None)
			{
				AlwaysRelevantNode->NotifyRemoveNetworkActor(ActorInfo);
			}
			else
			{
				FActorRepListRefView& RepList = AlwaysRelevantStreamingLevelActors.FindChecked(ActorInfo.StreamingLevelName);

				if (RepList.RemoveFast(ActorInfo.Actor) == false)
				{
					UE_LOG(LogS1RepGraph, Warning, TEXT("Actor %s was not found in AlwaysRelevantStreamingLevelActors list. LevelName: %s"), *GetActorRepListTypeDebugString(ActorInfo.Actor), *ActorInfo.StreamingLevelName.ToString());
				}
			}
			
			SetActorDestructionInfoToIgnoreDistanceCulling(ActorInfo.GetActor());

			break;
		}
		
		case EClassRepNodeMapping::Spatialize_Static:
		{
			GridNode->RemoveActor_Static(ActorInfo);
			break;
		}
		
		case EClassRepNodeMapping::Spatialize_Dynamic:
		{
			GridNode->RemoveActor_Dynamic(ActorInfo);
			break;
		}
		
		case EClassRepNodeMapping::Sptatialize_Dormancy:
		{
			GridNode->RemoveActor_Dormancy(ActorInfo);
			break;
		}
	}

	UE_LOG(LogS1RepGraph, Warning, TEXT("[RouteRemoveNetworkActorToNodes] : %s"), *ActorInfo.Actor->GetName());
}

/* @See UNetDriver::ProcessRemoteFunction(). ReplicationGraph::ProcessRemoteFunction essentially hijacks routing of MulticastRPC. */
/* @See below comment "Customize Condition of Routing MulticastRPC..." and This function completely overrides Super. */
bool US1ReplicationGraph::ProcessRemoteFunction(AActor* Actor, UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack, UObject* SubObject)
{
#if WITH_SERVER_CODE
	// ----------------------------------
	// Setup
	// ----------------------------------

	if (RepGraphConditionalActorBreakpoint(Actor, nullptr))
	{
		UE_LOG(LogReplicationGraph, Display, TEXT("UReplicationGraph::ProcessRemoteFunction: %s. Function: %s."), *GetNameSafe(Actor), *GetNameSafe(Function));
	}

	if (IsActorValidForReplication(Actor) == false || Actor->IsActorBeingDestroyed())
	{
		UE_LOG(LogReplicationGraph, Display, TEXT("UReplicationGraph::ProcessRemoteFunction: Actor %s destroyed or not ready! Function: %s."), *GetNameSafe(Actor), *GetNameSafe(Function));
		return true;
	}

	// get the top most function
	while (Function->GetSuperFunction())
	{
		Function = Function->GetSuperFunction();
	}

	// If we have a subobject, thats who we are actually calling this on. If no subobject, we are calling on the actor.
	UObject* TargetObj = SubObject ? SubObject : Actor;

	// Make sure this function exists for both parties.
	const FClassNetCache* ClassCache = NetDriver->NetCache->GetClassNetCache(TargetObj->GetClass());
	if (!ClassCache)
	{
		UE_LOG(LogReplicationGraph, Warning, TEXT("ClassNetCache empty, not calling %s::%s"), *Actor->GetName(), *Function->GetName());
		return true;
	}

	const FFieldNetCache* FieldCache = ClassCache->GetFromField(Function);
	if (!FieldCache)
	{
		UE_LOG(LogReplicationGraph, Warning, TEXT("FieldCache empty, not calling %s::%s"), *Actor->GetName(), *Function->GetName());
		return true;
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// FastShared Replication. This is ugly but the idea here is to just fill out the bunch parameters and return so that this bunch can be reused by other connections
	// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	if (FastSharedReplicationBunch && (FastSharedReplicationFuncName == Function->GetFName()))
	{
		// We also cache off a channel so we can call some of the serialization functions on it. This isn't really necessary though and we could break those parts off
		// into a static function.
		if (ensureMsgf(FastSharedReplicationChannel, TEXT("FastSharedReplicationPath set but FastSharedReplicationChannel is not! %s"), *Actor->GetPathName()))
		{
			// Reset the bunch here. It will be reused and we should only reset it right before we actually write to it.
			FastSharedReplicationBunch->Reset();

			// It sucks we have to a temp writer like this, but we don't know how big the payload will be until we serialize it
			FNetBitWriter TempWriter(nullptr, 0);

#if UE_NET_TRACE_ENABLED
			// Create trace collector if tracing is enabled for the target bunch
			FNetTraceCollector* Collector = GetTraceCollector(*FastSharedReplicationBunch);
			if (Collector)
			{
				Collector->Reset();
			}
			else
			{
				Collector = UE_NET_TRACE_CREATE_COLLECTOR(ENetTraceVerbosity::Trace);
				SetTraceCollector(*FastSharedReplicationBunch, Collector);
			}

			// We use the collector from the shared bunch
			SetTraceCollector(TempWriter, Collector);
#endif // UE_NET_TRACE_ENABLED

			TSharedPtr<FRepLayout> RepLayout = NetDriver->GetFunctionRepLayout(Function);
			RepLayout->SendPropertiesForRPC(Function, FastSharedReplicationChannel, TempWriter, Parameters);

			FNetBitWriter TempBlockWriter(nullptr, 0);

#if UE_NET_TRACE_ENABLED
			// Ugliness to get data reported correctly, we basically fold the data from TempWriter into TempBlockWriter and then to Bunch
			// Create trace collector if tracing is enabled for the target bunch			
			SetTraceCollector(TempBlockWriter, Collector ? UE_NET_TRACE_CREATE_COLLECTOR(ENetTraceVerbosity::Trace) : nullptr);
#endif // UE_NET_TRACE_ENABLED

			FastSharedReplicationChannel->WriteFieldHeaderAndPayload(TempBlockWriter, ClassCache, FieldCache, nullptr, TempWriter, true);

#if UE_NET_TRACE_ENABLED
			// As we have used the collector we need to reset it before injecting the final data
			if (Collector)
			{
				Collector->Reset();
			}
			UE_NET_TRACE_OBJECT_SCOPE(FastSharedReplicationChannel->ActorNetGUID, *FastSharedReplicationBunch, Collector, ENetTraceVerbosity::Trace);
#endif
			FastSharedReplicationChannel->WriteContentBlockPayload(TargetObj, *FastSharedReplicationBunch, false, TempBlockWriter);

			// Release temporary collector
			UE_NET_TRACE_DESTROY_COLLECTOR(GetTraceCollector(TempBlockWriter));

			FastSharedReplicationBunch = nullptr;
			FastSharedReplicationChannel = nullptr;
			FastSharedReplicationFuncName = NAME_None;
		}
		return true;
	}


	// ----------------------------------
	// Multicast
	// ----------------------------------

	if ((Function->FunctionFlags & FUNC_NetMulticast))
	{
		TSharedPtr<FRepLayout> RepLayout = NetDriver->GetFunctionRepLayout(Function);

		TOptional<FVector> ActorLocation;

		UNetDriver::ERemoteFunctionSendPolicy SendPolicy = UNetDriver::Default;
		if (CVar_RepGraph_EnableRPCSendPolicy > 0)
		{
			if (FRPCSendPolicyInfo* FuncSendPolicy = RPCSendPolicyMap.Find(FObjectKey(Function)))
			{
				if (FuncSendPolicy->bSendImmediately)
				{
					SendPolicy = UNetDriver::ForceSend;
				}
			}
		}

		RepLayout->BuildSharedSerializationForRPC(Parameters);
		FGlobalActorReplicationInfo& GlobalInfo = GlobalActorReplicationInfoMap.Get(Actor);

		bool ForceFlushNetDormancy = false;

		// Cache streaming level name off
		FNewReplicatedActorInfo NewActorInfo(Actor);
		const FName ActorStreamingLevelName = NewActorInfo.StreamingLevelName;
		EProcessRemoteFunctionFlags RemoteFunctionFlags = EProcessRemoteFunctionFlags::None;

		for (UNetReplicationGraphConnection* Manager : Connections)
		{
			FConnectionReplicationActorInfo& ConnectionActorInfo = Manager->ActorInfoMap.FindOrAdd(Actor);
			UNetConnection* NetConnection = Manager->NetConnection;

			// This connection isn't ready yet
			if (NetConnection->ViewTarget == nullptr)
			{
				continue;
			}

			// Streaming level actor that the client doesn't have loaded. Do not send.
			if (ActorStreamingLevelName != NAME_None && NetConnection->ClientVisibleLevelNames.Contains(ActorStreamingLevelName) == false)
			{
				continue;
			}

			/* Customize Condition of Routing MulticastRPC, using VisibilityBookkeeping.*/
			/* {{Actor, Manager->NetConnection->ViewTarget}, bool}* */
			if ((NetConnection->ViewTarget != Actor) && VisibilityBookkeeping.Contains({NetConnection->ViewTarget, Actor}) && * (VisibilityBookkeeping.Find({ NetConnection->ViewTarget, Actor })) == false)
			{ // should not process for this connection although Channel is valid, ViewTarget doesn't know about Actor in Client. Actor is Hiding.
				UE_LOG(LogS1RepGraph, Error, TEXT("[MulticastRPC is BLOCKED as Visibility]"));
				continue;
			}

			//UE_CLOG(ConnectionActorInfo.Channel == nullptr, LogReplicationGraph, Display, TEXT("Null channel on %s for %s"), *GetPathNameSafe(Actor), *GetNameSafe(Function));
			if (ConnectionActorInfo.Channel == nullptr && (RPC_Multicast_OpenChannelForClass.GetChecked(Actor->GetClass()) == true))
			{
				// There is no actor channel here. Ideally we would just ignore this but in the case of net dormancy, this may be an actor that will replicate on the next frame.
				// If the actor is dormant and is a distance culled actor, we can probably safely assume this connection will open a channel for the actor on the next rep frame.
				// This isn't perfect and we may want a per-function or per-actor policy that allows to dictate what happens in this situation.

				// Actors being destroyed (Building hit with rocket) will wake up before this gets hit. So dormancy really cant be relied on here.
				// if (Actor->NetDormancy > DORM_Awake)
				{
					bool bShouldOpenChannel = true;
					if (ConnectionActorInfo.GetCullDistanceSquared() > 0.f)
					{
						bShouldOpenChannel = false;
						if (ActorLocation.IsSet() == false)
						{
							ActorLocation = Actor->GetActorLocation();
						}

						FNetViewerArray ViewsToConsider;
						ViewsToConsider.Emplace(NetConnection, 0.f);

						for (int32 ChildIdx = 0; ChildIdx < NetConnection->Children.Num(); ++ChildIdx)
						{
							if (NetConnection->Children[ChildIdx]->ViewTarget != nullptr)
							{
								ViewsToConsider.Emplace(NetConnection->Children[ChildIdx], 0.f);
							}
						}

						// Loop through and see if we should keep this channel open, as when we do distance, we will
						// default to the channel being closed.
						for (const FNetViewer& Viewer : ViewsToConsider)
						{
							const float DistSq = (ActorLocation.GetValue() - Viewer.ViewLocation).SizeSquared();
							if (DistSq <= ConnectionActorInfo.GetCullDistanceSquared())
							{
								bShouldOpenChannel = true;
								break;
							}
						}
					}


					if (bShouldOpenChannel)
					{
#if !UE_BUILD_SHIPPING
						if (Actor->bOnlyRelevantToOwner && (!Actor->GetNetOwner() || (Actor->GetNetOwner() != NetConnection->PlayerController)))
						{
							UE_LOG(LogReplicationGraph, Warning, TEXT("Multicast RPC opening channel for bOnlyRelevantToOwner actor, check RPC_Multicast_OpenChannelForClass: Actor: %s Target: %s Function: %s"), *GetNameSafe(Actor), *GetNameSafe(TargetObj), *GetNameSafe(Function));
							ensureMsgf(Cast<APlayerController>(Actor) == nullptr, TEXT("MulticastRPC %s will open a channel for %s to a non-owner. This will break the PlayerController replication."), *Function->GetName(), *GetNameSafe(Actor));
						}
#endif

						// We are within range, we will open a channel now for this actor and call the RPC on it
						ConnectionActorInfo.Channel = (UActorChannel*)NetConnection->CreateChannelByName(NAME_Actor, EChannelCreateFlags::OpenedLocally);

						if (ConnectionActorInfo.Channel)
						{
							ConnectionActorInfo.Channel->SetChannelActor(Actor, ESetChannelActorFlags::None);

							// Update timeout frame name. We would run into problems if we open the channel, queue a bunch, and then it timeouts before RepGraph replicates properties.
							UpdateActorChannelCloseFrameNum(Actor, ConnectionActorInfo, GlobalInfo, GetReplicationGraphFrame() + 1 /** Plus one to error on safe side. RepFrame num will be incremented in the next tick */, NetConnection);

							// If this actor is dormant on the connection, we will force a flushnetdormancy call.
							ForceFlushNetDormancy |= ConnectionActorInfo.bDormantOnConnection;
						}
					}
				}
			}

			if (ConnectionActorInfo.Channel)
			{
				NetDriver->ProcessRemoteFunctionForChannel(ConnectionActorInfo.Channel, ClassCache, FieldCache, TargetObj, NetConnection, Function, Parameters, OutParms, Stack, true, SendPolicy, RemoteFunctionFlags);

				if (SendPolicy == UNetDriver::ForceSend)
				{
					// Queue the send in an array that we consume in PostTickDispatch to avoid force flushing multiple times a frame on the same connection
					ConnectionsNeedingsPostTickDispatchFlush.AddUnique(NetConnection);
				}

			}
		}

		RepLayout->ClearSharedSerializationForRPC();

		if (ForceFlushNetDormancy)
		{
			Actor->FlushNetDormancy();
		}
		return true;
	}

	// ----------------------------------
	// Single Connection
	// ----------------------------------

	UNetConnection* Connection = Actor->GetNetConnection();
	if (Connection)
	{
		const bool bIsReliable = EnumHasAnyFlags(Function->FunctionFlags, FUNC_NetReliable);

		// If we're saturated and it's not a reliable multicast, drop it.
		if (!(bIsReliable || IsConnectionReady(Connection)))
		{
			return true;
		}

		// Route RPC calls to actual connection
		if (Connection->GetUChildConnection())
		{
			Connection = ((UChildConnection*)Connection)->Parent;
		}

		if (Connection->GetConnectionState() == USOCK_Closed)
		{
			return true;
		}

		UActorChannel* Ch = Connection->FindActorChannelRef(Actor);
		if (Ch == nullptr)
		{
			if (Actor->IsPendingKillPending() || !NetDriver->IsLevelInitializedForActor(Actor, Connection))
			{
				// We can't open a channel for this actor here
				return true;
			}

			Ch = (UActorChannel*)Connection->CreateChannelByName(NAME_Actor, EChannelCreateFlags::OpenedLocally);
			Ch->SetChannelActor(Actor, ESetChannelActorFlags::None);

			if (UNetReplicationGraphConnection* ConnectionManager = Cast<UNetReplicationGraphConnection>(Connection->GetReplicationConnectionDriver()))
			{
				FConnectionReplicationActorInfo& ConnectionActorInfo = ConnectionManager->ActorInfoMap.FindOrAdd(Actor);
				FGlobalActorReplicationInfo& GlobalInfo = GlobalActorReplicationInfoMap.Get(Actor);
				UpdateActorChannelCloseFrameNum(Actor, ConnectionActorInfo, GlobalInfo, GetReplicationGraphFrame() + 1 /** Plus one to error on safe side. RepFrame num will be incremented in the next tick */, Connection);
			}
		}

		NetDriver->ProcessRemoteFunctionForChannel(Ch, ClassCache, FieldCache, TargetObj, Connection, Function, Parameters, OutParms, Stack, true);
	}
	else
	{
		UE_LOG(LogNet, Warning, TEXT("UReplicationGraph::ProcessRemoteFunction: No owning connection for actor %s. Function %s will not be processed."), *Actor->GetName(), *Function->GetName());
	}
#endif // WITH_SERVER_CODE

	// return true because we don't want the net driver to do anything else
	return true;
}

int32 US1ReplicationGraph::ServerReplicateActors(float DeltaSeconds)
{ /* Cache VisibleActors' WorldLocation. */
	CachedDeltaSeconds = DeltaSeconds;
	
	for (const FActorRepListType& VisibleActor : PotentiallyVisibleActorList)
	{
		FGlobalActorReplicationInfo& ActorRepInfo = GlobalActorReplicationInfoMap.Get(VisibleActor);
		ActorRepInfo.WorldLocation = VisibleActor->GetActorLocation();
	}

	return Super::ServerReplicateActors(DeltaSeconds);
}

/* ---------------------------------------- S1ReplicationGraph Code Segment End ---------------------------------------- */

void US1ReplicationGraphNode_AlwaysRelevant_ForConnection::ResetGameWorldState()
{
	ReplicationActorList.Reset();
	AlwaysRelevantStreamingLevelsNeedingReplication.Empty();
}

void US1ReplicationGraphNode_AlwaysRelevant_ForConnection::UpdateCachedRelevantActor(const FConnectionGatherActorListParameters& Params, AActor* NewActor, TWeakObjectPtr<AActor>& LastActor)
{
	if (NewActor != LastActor)
	{
		if (NewActor)
		{ // Zero out new actor cull distance
			Params.ConnectionManager.ActorInfoMap.FindOrAdd(NewActor).SetCullDistanceSquared(0.f);
		}

		if (AActor* PrevActor = LastActor.Get())
		{ // Reset previous actor culldistance
			FConnectionReplicationActorInfo& ActorInfo = Params.ConnectionManager.ActorInfoMap.FindOrAdd(PrevActor);
			ActorInfo.SetCullDistanceSquared(GraphGlobals->GlobalActorReplicationInfoMap->Get(PrevActor).Settings.GetCullDistanceSquared());
		}

		LastActor = NewActor;
	}
}

void US1ReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params)
{ /* Node마다 호출되는 함수. */
	US1ReplicationGraph* S1Graph = CastChecked<US1ReplicationGraph>(GetOuter());

	ReplicationActorList.Reset();

	for (const FNetViewer& CurViewer : Params.Viewers)
	{
		ReplicationActorList.ConditionalAdd(CurViewer.InViewer); // InViewer : PlayerController
		ReplicationActorList.ConditionalAdd(CurViewer.ViewTarget); // ViewTarget : Pawn

		if (ASpearmanPlayerController* PC = Cast<ASpearmanPlayerController>(CurViewer.InViewer))
		{
			// 50% throttling of PlayerStates.
			const bool bReplicatesPS = (Params.ConnectionManager.ConnectionOrderNum % 2) == (Params.ReplicationFrameNum % 2);
			if (bReplicatesPS)
			{
				// Always return the player state to the owning player.
				if (APlayerState* PS = PC->PlayerState)
				{ // PlayerState for Autonomous Proxy
					if (!bInitializePlayerState)
					{
						bInitializePlayerState = true;
						FConnectionReplicationActorInfo& ConnectionActorInfo = Params.ConnectionManager.ActorInfoMap.FindOrAdd(PS);
						ConnectionActorInfo.ReplicationPeriodFrame = 1;
					}

					ReplicationActorList.ConditionalAdd(PS);
				}
			}
			// TODO : 5.2 Lyra 코드보고 연결고리가 맞는지 다시 분석해보기
			FCachedAlwaysRelevantActorInfo& LastData = PastRelevantActorMap.FindOrAdd(CurViewer.Connection);
			
			if (ASpearmanCharacter* Pawn = Cast<ASpearmanCharacter>(PC->GetPawn()))
			{ /* UpdateCachedRelevantActor : AlwaysRelevant For Connection에 해당하는 Pawn이 변경되었다면 CullDistance를 0->기존값으로 다시 바꿔치기하는 것.*/
				UpdateCachedRelevantActor(Params, Pawn, LastData.LastViewer);

				if (Pawn != CurViewer.ViewTarget)
				{ /* 이건 조건에 관한 의미를 아직 모르겠음. 현재 Pawn과 ViewTarget이 달라졌나요? */
					ReplicationActorList.ConditionalAdd(Pawn);
				}

				if (AWeapon* Weapon = Pawn->GetCombat()->GetEquippedWeapon())
				{
					ReplicationActorList.ConditionalAdd(Weapon);
				}
			}

			if (ASpearmanCharacter* ViewTargetPawn = Cast<ASpearmanCharacter>(CurViewer.ViewTarget))
			{ /* 위랑 살짝 비슷한데, CurView->ViewTarget이 SpearmanCharacter가 맞다면 캐싱하는 것. */
				UpdateCachedRelevantActor(Params, ViewTargetPawn, LastData.LastViewTarget);
			}
		}
	}

	CleanupCachedRelevantActors(PastRelevantActorMap);
	Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorList);

	FPerConnectionActorInfoMap& ConnectionActorInfoMap = Params.ConnectionManager.ActorInfoMap;
	TMap<FName, FActorRepListRefView>& AlwaysRelevantStreamingLevelActors = S1Graph->AlwaysRelevantStreamingLevelActors;

	for (int32 Idx = AlwaysRelevantStreamingLevelsNeedingReplication.Num() - 1; Idx >= 0; --Idx)
	{
		const FName& StreamingLevel = AlwaysRelevantStreamingLevelsNeedingReplication[Idx];

		FActorRepListRefView* Ptr = AlwaysRelevantStreamingLevelActors.Find(StreamingLevel);
		if (Ptr == nullptr)
		{
			AlwaysRelevantStreamingLevelsNeedingReplication.RemoveAtSwap(Idx, 1, false);
			continue;
		}

		FActorRepListRefView& RepList = *Ptr;

		if (RepList.Num() > 0)
		{
			bool bAllDormant = true;

			for (FActorRepListType Actor : RepList)
			{
				FConnectionReplicationActorInfo& ConnectionActorInfo = ConnectionActorInfoMap.FindOrAdd(Actor);
				if (ConnectionActorInfo.bDormantOnConnection == false)
				{
					bAllDormant = false;
					break;
				}
			}

			if (bAllDormant)
			{ /* 해당 Level에 있는 Actor들이 전부 Dormant라면, 굳이 추가할 필요가 없겠죠? */
				UE_CLOG(S1::RepGraph::DisplayClientLevelStreaming > 0, LogS1RepGraph, Display, TEXT("CLIENTSTREAMING All AlwaysRelevant Actors Dormant on StreamingLevel %s for %s. Removing list."), *StreamingLevel.ToString(), *Params.ConnectionManager.GetName());
				AlwaysRelevantStreamingLevelsNeedingReplication.RemoveAtSwap(Idx, 1, false);
			}
			else /* 새롭게 로드된 StreamingLevel에서 AlwaysRelevant인 Actor들을 살펴보며 nullptr인지, 모두 dormant인지 확인하고, 아니라면 복제될 수있게 추가한다. */
			{ /* Dormant인 일부 Actor들까지 같이 추가되는데, 그건 어떻게 처리하는걸까? -> .cpp 1300(ReplicateActorListsForConnections_Default)에서 dormant라면 continue를 통해 추가안하고 거름.*/
				UE_CLOG(S1::RepGraph::DisplayClientLevelStreaming > 0, LogS1RepGraph, Display, TEXT("CLIENTSTREAMING Adding always Actors on StreamingLevel %s for %s because it has at least one non dormant actor"), *StreamingLevel.ToString(), *Params.ConnectionManager.GetName());
				Params.OutGatheredReplicationLists.AddReplicationActorList(RepList);
			}
		}
		else
		{
			UE_LOG(LogS1RepGraph, Warning, TEXT("US1ReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection - empty RepList %s"), *Params.ConnectionManager.GetName());
		}
	}

	// UE_LOG(LogS1RepGraph, Warning, TEXT("Called [GatherActorListsForConnection] !"));
}

void US1ReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityAdd(FName LevelName, UWorld* StreamingWorld)
{
	UE_CLOG(S1::RepGraph::DisplayClientLevelStreaming > 0, LogS1RepGraph, Display, TEXT("CLIENTSTREAMING ::OnClientLevelVisibilityAdd - %s"), *LevelName.ToString());
	AlwaysRelevantStreamingLevelsNeedingReplication.Add(LevelName);
}

void US1ReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityRemove(FName LevelName)
{
	UE_CLOG(S1::RepGraph::DisplayClientLevelStreaming > 0, LogS1RepGraph, Display, TEXT("CLIENTSTREAMING ::OnClientLevelVisibilityRemove - %s"), *LevelName.ToString());
	AlwaysRelevantStreamingLevelsNeedingReplication.Remove(LevelName);
}

US1ReplicationGraphNode_VisibilityCheck_ForConnection::US1ReplicationGraphNode_VisibilityCheck_ForConnection()
{
	bRequiresPrepareForReplicationCall = true;
}

void US1ReplicationGraphNode_VisibilityCheck_ForConnection::PrepareForReplication()
{
	/* Cache Controlled Pawn (Starting Point of Visibility Check) */
	CachedPawn = Cast<APawn>(ConnectionManager.Get()->NetConnection->ViewTarget);
}

/* US1ReplicationGraphNode_VisibilityCheck_ForConnection::GatherActorListsForConnection() - FogOfWar
*  This is naive implementation of FogOfWar. If you want optimized version, @See DynamicSpatialFrequency_VIsibilityCheck Node.
*	                # * <- BoundingBoxLeft (Upper/Lower)  *
*                #    | <- Latency Offset                 *
*             #       | <- Velocity Offset                *
* Trace    #          @ <- Default Offset                 *
* Start O-------------O TraceEnd                          *
*          #          @ <- Default Offset                 *
*             #       | <- Velocity Offset                *
*                #    | <- Latency Offset                 *
*                   # * <- BoundingBoxRight (Upper/Lower) *
* Trouble Shooting (1) : Weapon remains after culling.
* Answer : It's about NetLoadonClient. make sure NetLoadonClient = false and need to route Weapon as dormant and remove when unequipped.
* Trouble Shooting (2) : hiding SpearmanCharacter is visible when it uses MulticastRPC
* Answer : Add Condition in Routing of MulticastRPC. */
void US1ReplicationGraphNode_VisibilityCheck_ForConnection::GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params)
{
	US1ReplicationGraph* S1Graph = CastChecked<US1ReplicationGraph>(GetOuter());
	FGlobalActorReplicationInfoMap* GlobalRepMap = GraphGlobals.IsValid() ? GraphGlobals->GlobalActorReplicationInfoMap : nullptr;
	const FActorRepListRefView& VisibleActorList = S1Graph->PotentiallyVisibleActorList;
	ReplicationActorList.Reset();

	if (UNLIKELY(CachedPawn.Get() == nullptr || VisibleActorList.IsEmpty()))
	{ /* If true, MatchState::WaitingStart, Character is not spawned yet. */
		return;
	}

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CachedPawn.Get());
	const UWorld* World = GetWorld();
	const FVector TraceOffsetZ = FVector(0.f, 0.f, 80.f);
	const FVector TraceStart = CachedPawn->GetActorLocation() + TraceOffsetZ;

	for (AActor* ActorToCheck : VisibleActorList)
	{	
		if (CachedPawn.Get() == ActorToCheck)
		{
			continue;
		}
		
		// @FIXME : replace "4'000'000" with GetCullDistanceSquared()" < -have to change RepNodeMapping
		const FGlobalActorReplicationInfo& GlobalDataForActor = GlobalRepMap->Get(ActorToCheck);
		const float DistSq = ((GlobalDataForActor.WorldLocation + TraceOffsetZ) - TraceStart).SizeSquared();
		if (DistSq > 4'000'000)
		{ // Pre-culling
			continue;
		}
		/* Get Perpendicular Unit Vector to StartToEnd */
		const FVector TraceEnd = GlobalDataForActor.WorldLocation + TraceOffsetZ;
		const FVector NormalizedStartToEnd = (TraceEnd - TraceStart).GetSafeNormal();
		const FVector OffsetUnit = FVector(-NormalizedStartToEnd.Y, NormalizedStartToEnd.X, 0.f); // TOOD : Z factor

		const FVector DefaultOffset = 40.f * OffsetUnit;

		ASpearmanPlayerController* StartPC = CastChecked<ASpearmanCharacter>(CachedPawn.Get())->SpearmanPlayerController;
		ASpearmanPlayerController* EndPC = CastChecked<ASpearmanCharacter>(ActorToCheck)->SpearmanPlayerController;
		const float StartHalfRTT = StartPC ? StartPC->GetSingleTripTime() : 0.f;
		const float EndHalfRTT = EndPC ? EndPC->GetSingleTripTime() : 0.f;

		const FVector LatencyOffset = 2.f * (OffsetUnit * ActorToCheck->GetVelocity().GetAbs()) * (S1Graph->CachedDeltaSeconds + StartHalfRTT + EndHalfRTT);

		const FVector BoundingBoxLeftUpper = TraceEnd + FVector(0.f, 0.f, 20.f) - (DefaultOffset + LatencyOffset);
		const FVector BoundingBoxLeftLower = TraceEnd - FVector(0.f, 0.f, 130.f) - (DefaultOffset + LatencyOffset);
		const FVector BoundingBoxRightUpper = TraceEnd + FVector(0.f, 0.f, 20.f) + (DefaultOffset + LatencyOffset);
		const FVector BoundingBoxRightLower = TraceEnd - FVector(0.f, 0.f, 130.f) + (DefaultOffset + LatencyOffset);

		FHitResult HitResult;
		int32 ResultWriter = 0;
		ResultWriter += World->LineTraceSingleByChannel(HitResult, TraceStart, BoundingBoxLeftUpper, ECC_FogOfWar, TraceParams);
		ResultWriter += World->LineTraceSingleByChannel(HitResult, TraceStart, BoundingBoxLeftLower, ECC_FogOfWar, TraceParams);
		ResultWriter += World->LineTraceSingleByChannel(HitResult, TraceStart, BoundingBoxRightUpper, ECC_FogOfWar, TraceParams);
		ResultWriter += World->LineTraceSingleByChannel(HitResult, TraceStart, BoundingBoxRightLower, ECC_FogOfWar, TraceParams);
		S1Graph->LineTraceCounter += 4;

		if (ResultWriter < 4) // Visible
		{ 
			ReplicationActorList.Add(ActorToCheck);

			S1Graph->VisibilityBookkeeping.Add(TPair<AActor*, AActor*>(CachedPawn.Get(), ActorToCheck), true);
		}
		else // Hide
		{ 
			S1Graph->VisibilityBookkeeping.Add(TPair<AActor*, AActor*>(CachedPawn.Get(), ActorToCheck), false);
		}
	}

	Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorList);
}

/* -------------------------------- CVar From ReplicationGraph.cpp Start --------------------------------*/
REPGRAPH_DEVCVAR_SHIPCONST(int32, "Net.RepGraph.DynamicSpatialFrequency.UncapBandwidth", CVar_RepGraph_DynamicSpatialFrequency_UncapBandwidth, 0, "Testing CVar that uncaps bandwidth on UReplicationGraphNode_DynamicSpatialFrequency nodes.");
REPGRAPH_DEVCVAR_SHIPCONST(int32, "Net.RepGraph.DynamicSpatialFrequency.OpportunisticLoadBalance", CVar_RepGraph_DynamicSpatialFrequency_OpportunisticLoadBalance, 1, "Defers replication 1 frame in cases where many actors replicate on this frame but few on next frame.");

FORCEINLINE bool ReplicatesEveryFrame(const FConnectionReplicationActorInfo& ConnectionInfo, const bool CheckFastPath)
{
	return !(ConnectionInfo.ReplicationPeriodFrame > 1 && (!CheckFastPath || ConnectionInfo.FastPath_ReplicationPeriodFrame > 1));
}
/* -------------------------------- CVar From ReplicationGraph.cpp End --------------------------------*/

US1ReplicationGraphNode_DynamicSpatialFrequency_VisibilityCheck::US1ReplicationGraphNode_DynamicSpatialFrequency_VisibilityCheck()
{
	bRequiresPrepareForReplicationCall = true;
}

void US1ReplicationGraphNode_DynamicSpatialFrequency_VisibilityCheck::PrepareForReplication()
{
	/* Cache Controlled Pawn (Starting Point of Visibility Check) */
	CachedPawn = Cast<APawn>(ConnectionManager.Get()->NetConnection->ViewTarget);
}

void US1ReplicationGraphNode_DynamicSpatialFrequency_VisibilityCheck::GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params)
{
#if WITH_SERVER_CODE
	repCheck(GraphGlobals.IsValid());

	US1ReplicationGraph* S1RepGraph = Cast<US1ReplicationGraph>(GraphGlobals->ReplicationGraph);
	repCheck(S1RepGraph);
	repCheck(GraphGlobals->GlobalActorReplicationInfoMap);
	
	if (UNLIKELY(CachedPawn.Get() == nullptr))
	{ /* If true, MatchState::WaitingStart, Character is not spawned yet. */
		return;
	}
	/* Pull Persistent ActorList from S1RepGraph */
	const FActorRepListRefView& PotentiallyVisibleActorList = S1RepGraph->PotentiallyVisibleActorList;

	FGlobalActorReplicationInfoMap& GlobalMap = *GraphGlobals->GlobalActorReplicationInfoMap;
	UNetConnection* NetConnection = Params.ConnectionManager.NetConnection;
	FPerConnectionActorInfoMap& ConnectionActorInfoMap = Params.ConnectionManager.ActorInfoMap;
	const int32 FrameNum = Params.ReplicationFrameNum;
	int32 TotalNumActorsExpectedNextFrame = 0;
	int32& QueuedBits = NetConnection->QueuedBits;

	FSettings& MySettings = GetSettings();

	const int32 MaxNearestActors = MySettings.MaxNearestActors;

	// --------------------------------------------------------
	SortedReplicationList.Reset();
	NumExpectedReplicationsThisFrame = 0;
	NumExpectedReplicationsNextFrame = 0;

	bool DoFullGather = true;

	{
#if CSV_PROFILER
		FScopedCsvStatExclusive ScopedStat(CSVStatName);
#endif	

		// --------------------------------------------------------------------------------------------------------
		//	Two passes: filter list down to MaxNearestActors actors based on distance. Then calc freq and resort
		// --------------------------------------------------------------------------------------------------------
		if (MaxNearestActors >= 0 && !NetConnection->IsReplay())
		{
			int32 PossibleNumActors = PotentiallyVisibleActorList.Num();;

			// Are we even over the limit?
			for (const FStreamingLevelActorListCollection::FStreamingLevelActors& StreamingList : StreamingLevelCollection.StreamingLevelLists)
			{
				if (Params.CheckClientVisibilityForLevel(StreamingList.StreamingLevelName))
				{
					PossibleNumActors += StreamingList.ReplicationActorList.Num();
				}
			}

			if (PossibleNumActors > MaxNearestActors)
			{
				// We need to do an initial filtering pass over these actors based purely on distance (not time since last replicated, etc).
				// We will only replicate MaxNearestActors actors.
				QUICK_SCOPE_CYCLE_COUNTER(REPGRAPH_DynamicSpatialFrequency_Gather_WithCap);

				DoFullGather = false; // Don't do the full gather below. Just looking at SortedReplicationList is not enough because its possible no actors are due to replicate this frame.

				// Go through all lists, calc distance and cache FGlobalActorInfo*
				GatherActors_DistanceOnly(PotentiallyVisibleActorList, GlobalMap, ConnectionActorInfoMap, Params);

				for (const FStreamingLevelActorListCollection::FStreamingLevelActors& StreamingList : StreamingLevelCollection.StreamingLevelLists)
				{
					if (Params.CheckClientVisibilityForLevel(StreamingList.StreamingLevelName))
					{
						GatherActors_DistanceOnly(StreamingList.ReplicationActorList, GlobalMap, ConnectionActorInfoMap, Params);
					}
				}

				ensure(PossibleNumActors == SortedReplicationList.Num());

				// Sort list by distance, remove Num - MaxNearestActors from end
				SortedReplicationList.Sort();
				SortedReplicationList.SetNum(MaxNearestActors, false);

				// Do rest of normal spatial calculations and resort
				for (int32 idx = SortedReplicationList.Num() - 1; idx >= 0; --idx)
				{
					FDynamicSpatialFrequency_SortedItem& Item = SortedReplicationList[idx];
					AActor* Actor = Item.Actor;
					FGlobalActorReplicationInfo& GlobalInfo = *Item.GlobalInfo;

					CalcFrequencyForActor(Actor, S1RepGraph, NetConnection, GlobalInfo, ConnectionActorInfoMap, MySettings, Params.Viewers, FrameNum, idx);
				}

				SortedReplicationList.Sort();
			}
		}

		// --------------------------------------------------------------------------------------------------------
		//	Single pass: RepList -> Sorted frequency list. No cap on max number of actors to replicate
		// --------------------------------------------------------------------------------------------------------
		if (DoFullGather)
		{
			// No cap on numbers of actors, just pull them directly
			QUICK_SCOPE_CYCLE_COUNTER(REPGRAPH_DynamicSpatialFrequency_Gather);

			GatherActors(PotentiallyVisibleActorList, GlobalMap, ConnectionActorInfoMap, Params, NetConnection);

			for (const FStreamingLevelActorListCollection::FStreamingLevelActors& StreamingList : StreamingLevelCollection.StreamingLevelLists)
			{
				if (Params.CheckClientVisibilityForLevel(StreamingList.StreamingLevelName))
				{
					GatherActors(StreamingList.ReplicationActorList, GlobalMap, ConnectionActorInfoMap, Params, NetConnection);
				}
			}

			SortedReplicationList.Sort();
		}
	}

	// --------------------------------------------------------

	{
		QUICK_SCOPE_CYCLE_COUNTER(REPGRAPH_DynamicSpatialFrequency_Replicate);

		const int64 MaxBits = MySettings.MaxBitsPerFrame;
		int64 BitsWritten = 0;

		// This is how many "not every frame" actors we should replicate this frame. When assigning dynamic frequencies we also track who is due to rep this frame and next frame.
		// If this frame has more than the next frame expects, we will deffer half of those reps this frame. This will naturally tend to spread things out. It is not perfect, but low cost.
		// Note that when an actor is starved (missed a replication frame) they will not be counted for any of this.
		int32 OpportunisticLoadBalanceQuota = (NumExpectedReplicationsThisFrame - NumExpectedReplicationsNextFrame) >> 1;

		for (const FDynamicSpatialFrequency_SortedItem& Item : SortedReplicationList)
		{
			AActor* Actor = Item.Actor;
			FGlobalActorReplicationInfo& GlobalInfo = *Item.GlobalInfo;
			FConnectionReplicationActorInfo& ConnectionInfo = *Item.ConnectionInfo;

			if (!Actor || IsActorValidForReplication(Actor) == false)
			{
				continue;
			}


			if (RepGraphConditionalActorBreakpoint(Actor, NetConnection))
			{
				UE_LOG(LogReplicationGraph, Display, TEXT("UReplicationGraphNode_DynamicSpatialFrequency_Connection Replication: %s"), *Actor->GetName());
			}

			if (UNLIKELY(ConnectionInfo.bTearOff))
			{
				continue;
			}

			if (CVar_RepGraph_DynamicSpatialFrequency_OpportunisticLoadBalance && OpportunisticLoadBalanceQuota > 0 && Item.FramesTillReplicate == 0 && !ReplicatesEveryFrame(ConnectionInfo, Item.EnableFastPath))
			{
				//UE_LOG(LogReplicationGraph, Display, TEXT("[%d] Opportunistic Skip. %s. OpportunisticLoadBalanceQuota: %d (%d, %d)"), FrameNum, *Actor->GetName(), OpportunisticLoadBalanceQuota, NumExpectedReplicationsThisFrame, NumExpectedReplicationsNextFrame);
				OpportunisticLoadBalanceQuota--;
				continue;
			}

			// ------------------------------------------------------
			//	Default Replication
			// ------------------------------------------------------

			if (ReadyForNextReplication(ConnectionInfo, GlobalInfo, FrameNum))
			{
				ASpearmanCharacter* SpearmanCharacter = CastChecked<ASpearmanCharacter>(Actor);
				SpearmanCharacter->bReplicationNewPaused = CalcVisibilityForActor(Actor, GlobalInfo, S1RepGraph) ? false : true;
				// copy to trigger check pause replication, @See ReplicateActor() in ActorChannel
				TArray<FNetViewer>& ConnectionViewers = GetWorld()->GetWorldSettings()->ReplicationViewers;
				ConnectionViewers = Params.Viewers;
				/* ------------ Original Code start ------------ */
				BitsWritten += S1RepGraph->ReplicateSingleActor(Actor, ConnectionInfo, GlobalInfo, ConnectionActorInfoMap, Params.ConnectionManager, FrameNum);
				ConnectionInfo.FastPath_LastRepFrameNum = FrameNum; // Manually update this here, so that we don't fast rep next frame. When they line up, use default replication.
				/* ------------ Original Code end ------------ */
				// reset after rep (to reuse for connection)
				SpearmanCharacter->bReplicationNewPaused = false;
			}

			// ------------------------------------------------------
			//	Fast Path
			// ------------------------------------------------------

			else if (Item.EnableFastPath && ReadyForNextReplication_FastPath(ConnectionInfo, GlobalInfo, FrameNum))
			{
				const int64 FastSharedBits = S1RepGraph->ReplicateSingleActor_FastShared(Actor, ConnectionInfo, GlobalInfo, Params.ConnectionManager, FrameNum);
				QueuedBits -= FastSharedBits; // We are doing our own bandwidth limiting here, so offset the netconnection's tracking.
				BitsWritten += FastSharedBits;
			}

			// Bandwidth Cap
			if (BitsWritten > MaxBits && CVar_RepGraph_DynamicSpatialFrequency_UncapBandwidth == 0)
			{
				S1RepGraph->NotifyConnectionSaturated(Params.ConnectionManager);
				break;
			}
		}


		if (CVar_RepGraph_DynamicSpatialFrequency_UncapBandwidth > 0)
		{
			UE_LOG(LogS1RepGraph, Display, TEXT("Uncapped bandwidth usage of UReplicationGraphNode_DynamicSpatialFrequency = %d bits -> %d bytes -> %.2f KBytes/sec"), BitsWritten, (BitsWritten + 7) >> 3, ((float)((BitsWritten + 7) >> 3) / 1024.f) * GraphGlobals->ReplicationGraph->NetDriver->NetServerMaxTickRate);
		}
	}
#endif // WITH_SERVER_CODE
}

/*	                # * <- BoundingBoxLeft (Upper/Lower)  *
*                #    | <- Latency Offset                 *
*             #       | <- Velocity Offset                *
* Trace    #          @ <- Default Offset                 *
* Start O-------------O TraceEnd                          *
*          #          @ <- Default Offset                 *
*             #       | <- Velocity Offset                *
*                #    | <- Latency Offset                 *
*                   # * <- BoundingBoxRight (Upper/Lower) */
bool US1ReplicationGraphNode_DynamicSpatialFrequency_VisibilityCheck::CalcVisibilityForActor(AActor* ActorToCheck, const FGlobalActorReplicationInfo& GlobalInfoForActor, US1ReplicationGraph* S1RepGraph)
{ // true : Visible, false : Hidden
	if (UNLIKELY(CachedPawn.Get() == ActorToCheck))
	{
		UE_LOG(LogS1RepGraph, Error, TEXT("ActorToCheck is same as CachedPawn. not filtered. Check CalcFrequencyForActor()"));
		return false;
	}

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CachedPawn.Get());
	const UWorld* World = GetWorld();
	const FVector TraceOffsetZ = FVector(0.f, 0.f, 80.f);
	const FVector TraceStart = CachedPawn->GetActorLocation() + TraceOffsetZ;

	/* Get Perpendicular Unit Vector to StartToEnd */
	const FVector TraceEnd = GlobalInfoForActor.WorldLocation + TraceOffsetZ;
	const FVector NormalizedStartToEnd = (TraceEnd - TraceStart).GetSafeNormal();
	const FVector OffsetUnit = FVector(-NormalizedStartToEnd.Y, NormalizedStartToEnd.X, 0.f); // TOOD : Z factor

	const FVector DefaultOffset = 40.f * OffsetUnit;

	ASpearmanPlayerController* StartPC = CastChecked<ASpearmanCharacter>(CachedPawn.Get())->SpearmanPlayerController;
	ASpearmanPlayerController* EndPC = CastChecked<ASpearmanCharacter>(ActorToCheck)->SpearmanPlayerController;
	const float StartHalfRTT = StartPC ? StartPC->GetSingleTripTime() : 0.f;
	const float EndHalfRTT = EndPC ? EndPC->GetSingleTripTime() : 0.f;
	
	const FVector LatencyOffset = 2.f * (OffsetUnit * ActorToCheck->GetVelocity().GetAbs()) * (S1RepGraph->CachedDeltaSeconds + StartHalfRTT + EndHalfRTT);

	const FVector BoundingBoxLeftUpper = TraceEnd + FVector(0.f, 0.f, 20.f) - (DefaultOffset + LatencyOffset);
	const FVector BoundingBoxLeftLower = TraceEnd - FVector(0.f, 0.f, 130.f) - (DefaultOffset + LatencyOffset);
	const FVector BoundingBoxRightUpper = TraceEnd + FVector(0.f, 0.f, 20.f) + (DefaultOffset + LatencyOffset);
	const FVector BoundingBoxRightLower = TraceEnd - FVector(0.f, 0.f, 130.f) + (DefaultOffset + LatencyOffset);

	TArray<FVector> BoundingBoxes;
	BoundingBoxes.Reserve(4);
	BoundingBoxes.Add(BoundingBoxLeftUpper);
	BoundingBoxes.Add(BoundingBoxLeftLower);
	BoundingBoxes.Add(BoundingBoxRightUpper);
	BoundingBoxes.Add(BoundingBoxRightLower);

	for (const FVector& BoundingBox : BoundingBoxes)
	{
		S1RepGraph->LineTraceCounter++;
		if (!World->LineTraceTestByChannel(TraceStart, BoundingBox, ECC_FogOfWar, TraceParams))
		{
			// Visible, return : don't need to every 4 trace if early visible
			S1RepGraph->VisibilityBookkeeping.Add(TPair<AActor*, AActor*>(CachedPawn.Get(), ActorToCheck), true);
			return true;
		}
	}
	
	// Hidden
	S1RepGraph->VisibilityBookkeeping.Add(TPair<AActor*, AActor*>(CachedPawn.Get(), ActorToCheck), false);
	return false;
}


// Since we listen to global (static) events, we need to watch out for cross world broadcasts (PIE)
#if WITH_EDITOR
#define CHECK_WORLDS(X) if(X->GetWorld() != GetWorld()) return;
#else
#define CHECK_WORLDS(X)
#endif

void US1ReplicationGraph::OnCharacterSwapWeapon(ASpearmanCharacter* Character, AWeapon* NewWeapon, AWeapon* OldWeapon)
{	
	CHECK_WORLDS(Character);

	if (OldWeapon)
	{
		GlobalActorReplicationInfoMap.RemoveDependentActor(Character, OldWeapon);
		
		FGlobalActorReplicationInfo& GlobalInfo = GlobalActorReplicationInfoMap.Get(OldWeapon);
		RouteAddNetworkActorToNodes(FNewReplicatedActorInfo(OldWeapon), GlobalInfo);
	}

	if (NewWeapon)
	{
		GlobalActorReplicationInfoMap.AddDependentActor(Character, NewWeapon);
	
		RouteRemoveNetworkActorToNodes(FNewReplicatedActorInfo(NewWeapon));
	}

	UE_LOG(LogS1RepGraph, Warning, TEXT("Called [OnCharacterSwapWeapon] !"));
}

void US1ReplicationGraph::PrintRepNodePolicies()
{
	UEnum* Enum = StaticEnum<EClassRepNodeMapping>();
	if (!Enum)
	{
		return;
	}

	GLog->Logf(TEXT("===================================="));
	GLog->Logf(TEXT("S1 Replication Routing Policies"));
	GLog->Logf(TEXT("===================================="));

	for (auto It = ClassRepNodePolicies.CreateIterator(); It; ++It)
	{
		FObjectKey ObjKey = It.Key();

		EClassRepNodeMapping Mapping = It.Value();

		GLog->Logf(TEXT("%-40s --> %s"), *GetNameSafe(ObjKey.ResolveObjectPtr()), *Enum->GetNameStringByValue(static_cast<uint32>(Mapping)));
	}
}