using UnrealBuildTool;

public class ChopItTests : ModuleRules
{
	public ChopItTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ChopIt",
			"ChopItCore",
			"ChopItCombat",
			"ChopItAI",
			"ChopItWorld",
			"ChopItPresentation",
			"EnhancedInput",
			"GameplayTags",
			"GameplayCameras",
			"StateTreeModule",
			"InputCore",
			"Slate",
			"SlateCore",
			"UMG",
			"UnrealEd"
		});
	}
}
