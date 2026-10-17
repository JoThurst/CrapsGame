// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class CrapsCrapsCraps : ModuleRules
{
	public CrapsCrapsCraps(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] {
			ModuleDirectory,
			Path.Combine(ModuleDirectory, "Core"),
			Path.Combine(ModuleDirectory, "Dice"),
			Path.Combine(ModuleDirectory, "Pawn"),
			Path.Combine(ModuleDirectory, "Pawn", "ThrowStyles"),
			Path.Combine(ModuleDirectory, "Scoring"),
			Path.Combine(ModuleDirectory, "Table"),
			Path.Combine(ModuleDirectory, "Hazards"),
			Path.Combine(ModuleDirectory, "Items"),
			Path.Combine(ModuleDirectory, "Progression"),
			Path.Combine(ModuleDirectory, "UI"),
			Path.Combine(ModuleDirectory, "Tests")
		});

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"PhysicsCore",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore"
		});
	}
}

