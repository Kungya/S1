// Fill out your copyright notice in the Description page of Project Settings.


#include "S1ReplicationGraph.h"

#include "Net/UnrealNetwork.h"
#include "Engine/LevelStreaming.h"
#include "EngineUtils.h"
#include "CoreGlobals.h"

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
		UE_LOG(LogS1RepGraph, Error, TEXT("SpearmanCharacter's Policy is VisibilityCheck_ForConnection"));
		return EClassRepNodeMapping::VisibilityCheck_ForConnection;
	}

	AActor* ActorCDO = Cast<AActor>(Class->GetDefaultObject());
	if (!ActorCDO || !ActorCDO->GetIsReplicated())
	{
		return EClassRepNodeMapping::NotRouted;
	}

	auto ShouldSpatialize = [](const AActor* CDO)
	{ // Spatialize���� �������� �����ϴ� 3���� ���� �߿��� �ϳ��� �����ؾ� ��.
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
	/* �Ʒ� �ΰ��� lambda�� Map�� InitNewElement Functor�� �������ִ°ǵ�, �� Functor�� Get()���� ����. */
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
	{ /* config���� ������ ���������� �ܾ Class�� ���� ��å�� ���� �����ϴ� ��. */
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

	/* ���� �ٷ� �Ʒ��� �ִ� �͵��� explicitly�ϰ� �����ϴ� ���̰�, �Ʒ� AllReplicatedClass�� for loop�� ���� RegisterClassReplicationInf()�ϴ°� Legacy�� �ش�. */
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
	/* ACharacter�� �װ� ��ӹ޴� Ŭ�������� ���� ���� ReplicationInfo�� �����ڽ��ϴ� ��� ��������� ����.. */
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
	RPC_Multicast_OpenChannelForClass.Set(AController::StaticClass(), false); // multicasts should never open channels on Controllers since opening a channel on a non-owner breaks the Controller's replication.
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

	// TODO : PlayerState Frequency Limiter if using GAS

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
	US1ReplicationGraphNode_VisibilityCheck_ForConnection* VisibilityCheckConnectionNode = CreateNewNode<US1ReplicationGraphNode_VisibilityCheck_ForConnection>();
	
	AddConnectionGraphNode(VisibilityCheckConnectionNode, RepGraphConnection);

	VisibilityCheckForConnectionNodes.Add(RepGraphConnection->NetConnection, VisibilityCheckConnectionNode);
	VisibilityCheckConnectionNode->ConnectionManager = RepGraphConnection;

	UE_LOG(LogS1RepGraph, Warning, TEXT("Called [InitConnectionGraphNodes] !"));
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

void US1ReplicationGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo)
{
	EClassRepNodeMapping Policy = GetMappingPolicy(ActorInfo.Class);
	
	switch (Policy)
	{
		case EClassRepNodeMapping::NotRouted:
		{ /* i.e : AWeapon, APlayerController */
			break;
		}

		case EClassRepNodeMapping::VisibilityCheck_ForConnection:
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

	UE_LOG(LogS1RepGraph, Warning, TEXT("Called [RouteAddNetworkActorToNodes] !"));
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

		case EClassRepNodeMapping::VisibilityCheck_ForConnection:
		{
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
}

int32 US1ReplicationGraph::ServerReplicateActors(float DeltaSeconds)
{ /* Cache VisibleActors' WorldLocation. */
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
{ /* Node���� ȣ��Ǵ� �Լ�. */
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
			// TODO : 5.2 Lyra �ڵ庸�� ������� �´��� �ٽ� �м��غ���
			FCachedAlwaysRelevantActorInfo& LastData = PastRelevantActorMap.FindOrAdd(CurViewer.Connection);

			if (ASpearmanCharacter* Pawn = Cast<ASpearmanCharacter>(PC->GetPawn()))
			{ /* UpdateCachedRelevantActor : AlwaysRelevant For Connection�� �ش��ϴ� Pawn�� ����Ǿ��ٸ� CullDistance�� 0->���������� �ٽ� �ٲ�ġ���ϴ� ��.*/
				UpdateCachedRelevantActor(Params, Pawn, LastData.LastViewer);

				if (Pawn != CurViewer.ViewTarget)
				{ /* �̰� ���ǿ� ���� �ǹ̸� ���� �𸣰���. ���� Pawn�� ViewTarget�� �޶�������? */
					ReplicationActorList.ConditionalAdd(Pawn);
				}
			}

			if (ASpearmanCharacter* ViewTargetPawn = Cast<ASpearmanCharacter>(CurViewer.ViewTarget))
			{ /* ���� ��¦ ����ѵ�, CurView->ViewTarget�� SpearmanCharacter�� �´ٸ� ĳ���ϴ� ��. */
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
			{ /* �ش� Level�� �ִ� Actor���� ���� Dormant���, ���� �߰��� �ʿ䰡 ������? */
				UE_CLOG(S1::RepGraph::DisplayClientLevelStreaming > 0, LogS1RepGraph, Display, TEXT("CLIENTSTREAMING All AlwaysRelevant Actors Dormant on StreamingLevel %s for %s. Removing list."), *StreamingLevel.ToString(), *Params.ConnectionManager.GetName());
				AlwaysRelevantStreamingLevelsNeedingReplication.RemoveAtSwap(Idx, 1, false);
			}
			else /* ���Ӱ� �ε�� StreamingLevel���� AlwaysRelevant�� Actor���� ���캸�� nullptr����, ��� dormant���� Ȯ���ϰ�, �ƴ϶�� ������ ���ְ� �߰��Ѵ�. */
			{ /* Dormant�� �Ϻ� Actor����� ���� �߰��Ǵµ�, �װ� ��� ó���ϴ°ɱ�? -> .cpp 1300(ReplicateActorListsForConnections_Default)���� dormant��� continue�� ���� �߰����ϰ� �Ÿ�.*/
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

void US1ReplicationGraphNode_VisibilityCheck_ForConnection::GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params)
{ // Check Visibility by Raycasting
	US1ReplicationGraph* S1Graph = CastChecked<US1ReplicationGraph>(GetOuter());
	FGlobalActorReplicationInfoMap* GlobalRepMap = GraphGlobals.IsValid() ? GraphGlobals->GlobalActorReplicationInfoMap : nullptr;
	const FActorRepListRefView& VisibleActorList = S1Graph->PotentiallyVisibleActorList;
	/*
	* Trouble Shooting : Weapon is early Visible, and following SpearmanCharacter.
	* TODO : Find why
	*/
	if (UNLIKELY(CachedPawn.Get() == nullptr || VisibleActorList.IsEmpty()))
	{ /* If this, MatchState::WaitingStart, not spawning yet. */
		return;
	}

	ReplicationActorList.Reset();

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CachedPawn.Get());
	const UWorld* World = GetWorld();
	const FVector TraceOffsetZ = FVector(0.f, 0.f, 180.f);
	const FVector TraceStart = CachedPawn->GetActorLocation() + TraceOffsetZ;

	for (const auto& ActorToCheck : VisibleActorList)
	{	
		if (CachedPawn.Get() == ActorToCheck)
		{
			continue;
		}
		
		const FGlobalActorReplicationInfo& GlobalDataForActor = GlobalRepMap->Get(ActorToCheck);
		const float DistSq = (GlobalDataForActor.WorldLocation - TraceStart).SizeSquared();
		
		// 20m, @FIXME : replace "4'000'000" with "GlobalRepMap->GetClassInfo().GetCullDistanceSquared()"
		if (DistSq > 4'000'000)
		{
			continue;
		}

		const FVector TraceEnd = GlobalDataForActor.WorldLocation + TraceOffsetZ;

		FHitResult HitResult;
		World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_FogOfWar, TraceParams);
		if (HitResult.bBlockingHit == false)
		{ // Visible
			ReplicationActorList.Add(ActorToCheck);
		}
	}

	Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorList);
}


// Since we listen to global (static) events, we need to watch out for cross world broadcasts (PIE)
#if WITH_EDITOR
#define CHECK_WORLDS(X) if(X->GetWorld() != GetWorld()) return;
#else
#define CHECK_WORLDS(X)
#endif

void US1ReplicationGraph::OnCharacterSwapWeapon(ASpearmanCharacter* Character, AWeapon* NewWeapon, AWeapon* OldWeapon)
{
	if (Character == nullptr) return;
	
	CHECK_WORLDS(Character);

	if (NewWeapon && OldWeapon)
	{ // Swap (Equip and UnEquip)
		GlobalActorReplicationInfoMap.RemoveDependentActor(Character, OldWeapon);
		GlobalActorReplicationInfoMap.AddDependentActor(Character, NewWeapon);
	}
	else if (NewWeapon && !OldWeapon)
	{ // Equip
		GlobalActorReplicationInfoMap.AddDependentActor(Character, NewWeapon);
	}
	else if (!NewWeapon && OldWeapon)
	{ // UnEquip
		GlobalActorReplicationInfoMap.RemoveDependentActor(Character, OldWeapon);
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