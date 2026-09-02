#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Visual/ChopItVisualStylePreset.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItVisualStyleAssetsTest,
	"ChopIt.Presentation.VisualStyle.Assets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItVisualStyleAssetsTest::RunTest(const FString& Parameters)
{
	const TArray<FString> RequiredPackages =
	{
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/M_LowPoly_Master_Authored"),
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/M_PP_ChopItVisualStyle"),
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Outline/M_OutlineOverlay"),
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Outline/MI_Outline_Global"),
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Outline/MI_Outline_Player"),
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Outline/MI_Outline_Enemy"),
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Outline/MI_Outline_Interactable"),
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Instances/MI_LowPoly_Forest"),
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Instances/MI_LowPoly_Industrial"),
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Instances/MI_LowPoly_Magic"),
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Presets/DA_VisualPreset_Megabonk"),
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Presets/DA_VisualPreset_MachineParty"),
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Presets/DA_VisualPreset_Hybrid"),
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Textures/T_PixelChecker_8"),
		TEXT("/Game/ChopIt/World/Maps/L_VisualStyleDemo")
	};
	for (const FString& Package : RequiredPackages)
	{
		TestTrue(FString::Printf(TEXT("Visual style package exists: %s"), *Package), FPackageName::DoesPackageExist(Package));
	}

	UMaterial* Master = LoadObject<UMaterial>(nullptr, TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/M_LowPoly_Master_Authored.M_LowPoly_Master_Authored"));
	TestNotNull(TEXT("Low-poly master material loads"), Master);
	if (Master)
	{
		TestEqual(TEXT("Master is a surface material"), Master->MaterialDomain.GetValue(), MD_Surface);
		TestTrue(TEXT("Master supports Nanite usage"), Master->GetUsageByFlag(MATUSAGE_Nanite));
		TestTrue(TEXT("Master supports instanced static meshes"), Master->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
	}

	UMaterial* PostProcess = LoadObject<UMaterial>(nullptr, TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/M_PP_ChopItVisualStyle.M_PP_ChopItVisualStyle"));
	TestNotNull(TEXT("Post-process material loads"), PostProcess);
	if (PostProcess)
	{
		TestEqual(TEXT("Post-process domain"), PostProcess->MaterialDomain.GetValue(), MD_PostProcess);
		TestEqual(TEXT("Post-process uses the UE 5.8 scene-color/depth pass"), PostProcess->BlendableLocation.GetValue(), BL_SceneColorBeforeBloom);
	}

	UMaterial* OutlineOverlay = LoadObject<UMaterial>(nullptr, TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Outline/M_OutlineOverlay.M_OutlineOverlay"));
	TestNotNull(TEXT("Outline overlay material loads"), OutlineOverlay);
	if (OutlineOverlay)
	{
		TestEqual(TEXT("Outline overlay is masked"), OutlineOverlay->BlendMode.GetValue(), BLEND_Masked);
		TestTrue(TEXT("Outline overlay renders its inverted hull"), OutlineOverlay->IsTwoSided());
		TestTrue(TEXT("Outline overlay supports instanced static meshes"), OutlineOverlay->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
	}

	const TCHAR* RestyledMaterials[] =
	{
		TEXT("MI_Ground"), TEXT("MI_Path"), TEXT("MI_Wood"), TEXT("MI_Trunk"), TEXT("MI_Leaves"), TEXT("MI_Roof"),
		TEXT("MI_Stone"), TEXT("MI_Player"), TEXT("MI_Enemy"), TEXT("MI_Interactable")
	};
	for (const TCHAR* Name : RestyledMaterials)
	{
		const FString Path = FString::Printf(TEXT("/Game/ChopIt/World/Blockout/Materials/%s.%s"), Name, Name);
		UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *Path);
		TestNotNull(FString::Printf(TEXT("Restyled blockout material loads: %s"), Name), Instance);
		if (Instance && Master)
		{
			TestEqual(FString::Printf(TEXT("%s uses the low-poly master"), Name), Instance->GetMaterial(), Master);
		}
	}

	UTexture2D* PixelTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/ChopIt/Presentation/VisualStyle/Textures/T_PixelChecker_8.T_PixelChecker_8"));
	TestNotNull(TEXT("Pixel demo texture loads"), PixelTexture);
	if (PixelTexture)
	{
		TestEqual(TEXT("Pixel demo texture uses nearest filtering"), PixelTexture->Filter.GetValue(), TF_Nearest);
		TestEqual(TEXT("Pixel demo texture has no generated mips"), PixelTexture->MipGenSettings.GetValue(), TMGS_NoMipmaps);
	}

	const TCHAR* AuthoredTextures[] =
	{
		TEXT("T_Ground_Detailed"), TEXT("T_Grass_Fern"), TEXT("T_Ground_Mossy"), TEXT("T_Grass_Soft"),
		TEXT("T_Leaves_Canopy"), TEXT("T_Stone_Cobble"), TEXT("T_Stone_LowPoly"), TEXT("T_Wood_Planks"), TEXT("T_Log_Bark")
	};
	for (const TCHAR* Name : AuthoredTextures)
	{
		const FString Path = FString::Printf(TEXT("/Game/ChopIt/Presentation/VisualStyle/Textures/Authored/%s.%s"), Name, Name);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
		TestNotNull(FString::Printf(TEXT("Authored texture loads: %s"), Name), Texture);
		if (Texture)
		{
			TestEqual(FString::Printf(TEXT("%s uses nearest filtering"), Name), Texture->Filter.GetValue(), TF_Nearest);
			TestEqual(FString::Printf(TEXT("%s has no generated mips"), Name), Texture->MipGenSettings.GetValue(), TMGS_NoMipmaps);
		}
	}

	const UChopItVisualStylePreset* Megabonk = LoadObject<UChopItVisualStylePreset>(nullptr, TEXT("/Game/ChopIt/Presentation/VisualStyle/Presets/DA_VisualPreset_Megabonk.DA_VisualPreset_Megabonk"));
	const UChopItVisualStylePreset* Machine = LoadObject<UChopItVisualStylePreset>(nullptr, TEXT("/Game/ChopIt/Presentation/VisualStyle/Presets/DA_VisualPreset_MachineParty.DA_VisualPreset_MachineParty"));
	const UChopItVisualStylePreset* Hybrid = LoadObject<UChopItVisualStylePreset>(nullptr, TEXT("/Game/ChopIt/Presentation/VisualStyle/Presets/DA_VisualPreset_Hybrid.DA_VisualPreset_Hybrid"));
	TestNotNull(TEXT("Megabonk preset loads"), Megabonk);
	TestNotNull(TEXT("Machine Party preset loads"), Machine);
	TestNotNull(TEXT("Hybrid preset loads"), Hybrid);
	if (Megabonk && Machine && Hybrid)
	{
		TestTrue(TEXT("Megabonk is the most saturated preset"), Megabonk->Saturation > Hybrid->Saturation && Hybrid->Saturation > Machine->Saturation);
		TestTrue(TEXT("Hybrid uses dark manual exposure"), Hybrid->ExposureBias <= -0.6f);
		TestTrue(TEXT("Hybrid applies display-referred dark gain"), Hybrid->ColorGain < 0.9f);
		TestTrue(TEXT("Hybrid global outline is enabled"), Hybrid->bGlobalOutline);
		TestTrue(TEXT("Hybrid global outline remains thin"), Hybrid->GlobalOutlineThickness <= 1.1f);
		TestTrue(TEXT("Hybrid global outline is near black"), Hybrid->GlobalOutlineColor.GetLuminance() < 0.03f);
		TestTrue(TEXT("Megabonk global outline is enabled"), Megabonk->bGlobalOutline);
		TestTrue(TEXT("Machine Party has the strongest grain"), Machine->GrainIntensity > Hybrid->GrainIntensity && Hybrid->GrainIntensity > Megabonk->GrainIntensity);
		TestTrue(TEXT("Machine Party has the strongest vignette"), Machine->VignetteIntensity > Hybrid->VignetteIntensity);
		TestFalse(TEXT("Hybrid scanlines stay disabled by default"), Hybrid->bScanlines);
		TestFalse(TEXT("Hybrid distortion stays disabled by default"), Hybrid->bScreenDistortion);
		TestFalse(TEXT("Hybrid depth fog stays disabled to avoid duplicating height fog"), Hybrid->bDepthFog);
		TestTrue(TEXT("Hybrid uses dense teal height fog"), Hybrid->FogDensity >= 0.035f && Hybrid->FogColor.B > Hybrid->FogColor.R);
		TestEqual(TEXT("Hybrid fog starts beyond the immediate foreground"), Hybrid->FogStartDistance, 250.0f);
		TestTrue(TEXT("Hybrid fog has a capped opacity"), Hybrid->FogMaxOpacity < 1.0f);
	}
	return true;
}

#endif
