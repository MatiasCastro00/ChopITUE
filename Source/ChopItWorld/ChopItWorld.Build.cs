using UnrealBuildTool;

public class ChopItWorld : ModuleRules
{
	public ChopItWorld(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] { "Chaos", "PhysicsCore" });
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "ChopItCombat", "ChopItCore", "ChopItPresentation" });
	}
}
