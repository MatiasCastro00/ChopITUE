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
			"AssetTools",
			"ChopIt",
			"ChopItCore",
			"ChopItCombat",
			"ChopItAI",
			"ChopItWorld",
			"ChopItPresentation",
			"GameplayCameras",
			"GameplayCamerasEditor",
			"GameplayTags",
			"StateTreeModule",
			"StateTreeEditorModule",
			"PropertyBindingUtils",
			"EnhancedInput",
			"InputCore",
			"KismetCompiler",
			"MaterialEditor",
			"NavigationSystem",
			"UnrealEd",
			"Projects"
		});
	}
}
