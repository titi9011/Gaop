// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Goap : ModuleRules
{
	public Goap(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Permet les includes avec sous-dossiers : "GOAP/GOAPAction.h", "Vehicle/GOAPVehicle.h"...
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore",
			"HeadMountedDisplay", "EnhancedInput",
			"AIModule",   // AAIController
		});
	}
}
