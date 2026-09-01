using UnrealBuildTool;

public class ChopItPresentation : ModuleRules
{
	public ChopItPresentation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayCameras",
			"GameplayTags",
			"StateTreeModule",
			"UMG",
			"ChopItCore"
		});
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ChopItCombat",
			"EnhancedInput",
			"Slate",
			"SlateCore"
		});
	}
}
