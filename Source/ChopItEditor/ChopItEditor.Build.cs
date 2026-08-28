using UnrealBuildTool;

public class ChopItEditor : ModuleRules
{
	public ChopItEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"ChopIt",
			"ChopItCore",
			"ChopItCombat",
			"ChopItAI",
			"ChopItWorld",
			"EnhancedInput",
			"InputCore",
			"KismetCompiler",
			"NavigationSystem",
			"UnrealEd",
			"Projects"
		});
	}
}
