// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ChopIt : ModuleRules
{
	public ChopIt(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"ChopItCombat",
			"ChopItWorld",
			"ChopItAI",
			"ChopItPresentation"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EnhancedInput",
			"GameplayTags",
			"GameplayCameras",
			"InputCore",
			"ChopItCore",
			"ChopItMeta"
		});
	}
}
