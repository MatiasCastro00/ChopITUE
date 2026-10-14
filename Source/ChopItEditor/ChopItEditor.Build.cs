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
			"GeometryCore",
			"GeometryFramework",
			"MeshConversion", "MeshDescription", "StaticMeshDescription",
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
			"Json",
			"JsonUtilities",
			"MaterialEditor",
			"RHI",
			"RenderCore",
			"PropertyEditor",
			"ToolMenus",
			"NavigationSystem",
			"UnrealEd",
			"UMG",
			"MovieScene", "MovieSceneTracks",
			"UMGEditor",
			"Slate",
			"SlateCore",
			"Projects"
		});
	}
}
