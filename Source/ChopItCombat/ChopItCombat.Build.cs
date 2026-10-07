using UnrealBuildTool;

public class ChopItCombat : ModuleRules
{
	public ChopItCombat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "GameplayTags" });
		PrivateDependencyModuleNames.AddRange(new string[] { "ChopItCore" });
	}
}
