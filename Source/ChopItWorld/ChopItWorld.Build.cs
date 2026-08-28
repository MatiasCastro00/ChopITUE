using UnrealBuildTool;

public class ChopItWorld : ModuleRules
{
	public ChopItWorld(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "ChopItCombat", "ChopItCore", "ChopItPresentation" });
	}
}
