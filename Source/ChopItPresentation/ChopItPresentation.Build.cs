using UnrealBuildTool;

public class ChopItPresentation : ModuleRules
{
	public ChopItPresentation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new string[] { "ChopItCore", "ChopItCombat" });
	}
}
