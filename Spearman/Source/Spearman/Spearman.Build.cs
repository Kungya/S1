// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Spearman : ModuleRules
{
	public Spearman(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG", "NavigationSystem", "AIModule", "GameplayTasks", "GameplayTags", "Paper2D", "OnlineSubsystemSteam", "OnlineSubsystem", "MultiplayerSubsystem", "NetCore" });

        PrivateDependencyModuleNames.AddRange(new string[] { "ReplicationGraph" });

		// Uncomment if you are using Slate UI
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
