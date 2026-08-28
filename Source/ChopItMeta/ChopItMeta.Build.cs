using UnrealBuildTool;

public class ChopItMeta : ModuleRules
{
	public ChopItMeta(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.Add("ChopItCore");
	}
}
