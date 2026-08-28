using UnrealBuildTool;

public class ChopItAI : ModuleRules
{
	public ChopItAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "ChopItWorld", "ChopItPresentation" });
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ChopItCore",
			"ChopItCombat",
			"AIModule",
			"NavigationSystem",
			"GameplayTags"
		});
	}
}
