using UnrealBuildTool;

public class ChopItWorld : ModuleRules
{
	public ChopItWorld(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "DeveloperSettings", "Json", "JsonUtilities", "ChopItCombat", "ChopItCore", "ChopItPresentation", "GeometryCore", "GeometryFramework", "Niagara" });
		PrivateDependencyModuleNames.Add("UMG");
	}
}
