// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ReplicationGraphTypes.h"
#include "S1ReplicationGraphTypes.generated.h"

UENUM()
enum class EClassRepNodeMapping : uint32
{
	NotRouted,
	RelevantAllConnections, // Routes to an AlwaysRelevantNode or AlwaysRelevantStreamingLevelNode node

	// --------------- ONLY SPATIALIZED Enums below here ! ----------------

	Spatialize_Static,
	Spatialize_Dynamic,
	Sptatialize_Dormancy,
};

USTRUCT()
struct FRepGraphActorClassSettings
{
	GENERATED_BODY()
	
	FRepGraphActorClassSettings() = default;
	
	// Name of the Class the settings will be applied to
	UPROPERTY(EditAnywhere)
	FSoftClassPath ActorClass;

	// If we should add this Class' RepInfo to the ClassRepNodePolicies Map	
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bAddClassRepInfoToMap = true;

	// What ClassNodeMapping we should use when adding Class to ClassRepNodePolicies Map
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bAddClassRepInfoToMap"))
	EClassRepNodeMapping ClassNodeMapping = EClassRepNodeMapping::NotRouted;
	
	// Should we add this to the RPC_Multicast_OpenChannelForClas map
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bAddToRPC_Multicast_OpenChannelForClassMap = false;
	
	// If this is added to to RPC_Multicast_OpenChnnelForClass map then should we actually open a channel or not
	UPROPERTY(EditAnywhere)
	bool bRPC_Multicast_OpenChannelForClass = true;	
	
	UClass* GetStaticActorClass() const
	{
		UClass* StaticActorClass = nullptr;
		const FString ActorClassNameString = ActorClass.ToString();

		if (FPackageName::IsScriptPackage(ActorClassNameString))
		{
			StaticActorClass = FindObject<UClass>(nullptr, *ActorClassNameString, true);

			if (!StaticActorClass)
			{
				UE_LOG(LogTemp, Error, TEXT("FRepGraphActorClassSettings: Cannot Find Static Class for %s"), *ActorClassNameString);
			}
		}
		else
		{
			// Allow blueprints to be used for custom class settings
			StaticActorClass = (UClass*)StaticLoadObject(UClass::StaticClass(), nullptr, *ActorClassNameString);
			if (!StaticActorClass)
			{
				UE_LOG(LogTemp, Error, TEXT("FRepGraphActorClassSettings: Cannot Load Static Class for %s"), *ActorClassNameString);
			}
		}

		return StaticActorClass;
	}
};
