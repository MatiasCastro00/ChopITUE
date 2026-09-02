#include "Commandlets/ChopItBootstrapCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Framework/ChopItGameMode.h"
#include "Enemies/ChopItEnemyCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionTwoSidedSign.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Shop/ChopItShopTerminal.h"
#include "ShaderCompiler.h"
#include "UObject/SavePackage.h"
#include "Visual/ChopItVisualStylePreset.h"

namespace ChopItVisualBootstrap
{
	template <typename AssetType>
	AssetType* LoadOrCreate(const FString& PackageName, const FName AssetName)
	{
		UPackage* Package = FPackageName::DoesPackageExist(PackageName)
			? LoadPackage(nullptr, *PackageName, LOAD_None)
			: CreatePackage(*PackageName);
		if (!Package) return nullptr;
		if (AssetType* Existing = FindObject<AssetType>(Package, *AssetName.ToString())) return Existing;
		AssetType* Asset = NewObject<AssetType>(Package, AssetName, RF_Public | RF_Standalone);
		if (Asset) FAssetRegistryModule::AssetCreated(Asset);
		return Asset;
	}

	bool Save(UObject* Asset)
	{
		if (!Asset) return false;
		UPackage* Package = Asset->GetOutermost();
		Package->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		Args.Error = GError;
		return UPackage::SavePackage(
			Package,
			Asset,
			*FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()),
			Args);
	}

	UMaterialExpressionScalarParameter* Scalar(
		UMaterial* Material,
		const TCHAR* Name,
		const float Default,
		const FName Group,
		const int32 X,
		const int32 Y,
		const float Min = 0.0f,
		const float Max = 1.0f)
	{
		auto* Expression = CastChecked<UMaterialExpressionScalarParameter>(
			UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionScalarParameter::StaticClass(), X, Y));
		Expression->ParameterName = Name;
		Expression->DefaultValue = Default;
		Expression->Group = Group;
		Expression->SliderMin = Min;
		Expression->SliderMax = Max;
		return Expression;
	}

	UMaterialExpressionVectorParameter* Vector(
		UMaterial* Material,
		const TCHAR* Name,
		const FLinearColor& Default,
		const FName Group,
		const int32 X,
		const int32 Y)
	{
		auto* Expression = CastChecked<UMaterialExpressionVectorParameter>(
			UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionVectorParameter::StaticClass(), X, Y));
		Expression->ParameterName = Name;
		Expression->DefaultValue = Default;
		Expression->Group = Group;
		return Expression;
	}

	void Input(UMaterialExpressionCustom* Custom, const TCHAR* Name, UMaterialExpression* Expression, const int32 Output = 0)
	{
		FCustomInput& CustomInput = Custom->Inputs.AddDefaulted_GetRef();
		CustomInput.InputName = Name;
		CustomInput.Input.Connect(Output, Expression);
	}

	UTexture2D* CreatePixelTexture()
	{
		UTexture2D* Texture = LoadOrCreate<UTexture2D>(
			TEXT("/Game/ChopIt/Presentation/VisualStyle/Textures/T_PixelChecker_8"),
			TEXT("T_PixelChecker_8"));
		if (!Texture) return nullptr;
		Texture->Source.Init(8, 8, 1, 1, TSF_BGRA8);
		uint8* Pixels = Texture->Source.LockMip(0);
		for (int32 Y = 0; Y < 8; ++Y)
		{
			for (int32 X = 0; X < 8; ++X)
			{
				const uint8 Pattern[] = { 255, 226, 242, 205 };
				const uint8 Value = Pattern[(X + Y * 3 + (X / 2)) & 3];
				const int32 Index = (Y * 8 + X) * 4;
				Pixels[Index + 0] = Value;
				Pixels[Index + 1] = Value;
				Pixels[Index + 2] = Value;
				Pixels[Index + 3] = 255;
			}
		}
		Texture->Source.UnlockMip(0);
		Texture->Filter = TF_Nearest;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->AddressX = TA_Wrap;
		Texture->AddressY = TA_Wrap;
		Texture->CompressionSettings = TC_Default;
		Texture->SRGB = true;
		Texture->NeverStream = true;
		Texture->PostEditChange();
		return Save(Texture) ? Texture : nullptr;
	}

	bool ImportAuthoredSurfaceTextures()
	{
		struct FTextureSpec { const TCHAR* SourceName; const TCHAR* AssetName; };
		const FTextureSpec Specs[] =
		{
			{ TEXT("Grass.png"),  TEXT("T_Ground_Detailed") },
			{ TEXT("Grass2.png"), TEXT("T_Grass_Fern") },
			{ TEXT("Grass3.png"), TEXT("T_Ground_Mossy") },
			{ TEXT("Grass4.png"), TEXT("T_Grass_Soft") },
			{ TEXT("Leafs.png"),  TEXT("T_Leaves_Canopy") },
			{ TEXT("Stone.png"),  TEXT("T_Stone_Cobble") },
			{ TEXT("Stone2.png"), TEXT("T_Stone_LowPoly") },
			{ TEXT("Wood.png"),   TEXT("T_Wood_Planks") },
			{ TEXT("Log.png"),    TEXT("T_Log_Bark") }
		};

		const FString SourceDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("SourceArt/VisualStyle/Materials"));
		const FString DestinationPath = TEXT("/Game/ChopIt/Presentation/VisualStyle/Textures/Authored");
		TArray<UAssetImportTask*> Tasks;
		for (const FTextureSpec& Spec : Specs)
		{
			const FString Filename = FPaths::Combine(SourceDirectory, Spec.SourceName);
			if (!FPaths::FileExists(Filename))
			{
				UE_LOG(LogTemp, Error, TEXT("Missing authored texture source: %s"), *Filename);
				return false;
			}
			UAssetImportTask* Task = NewObject<UAssetImportTask>();
			Task->Filename = Filename;
			Task->DestinationPath = DestinationPath;
			Task->DestinationName = Spec.AssetName;
			Task->bAutomated = true;
			Task->bSave = false;
			Task->bReplaceExisting = true;
			Task->bReplaceExistingSettings = true;
			Task->bAsync = false;
			Tasks.Add(Task);
		}
		FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		AssetTools.Get().ImportAssetTasks(Tasks);

		bool bSuccess = true;
		for (const FTextureSpec& Spec : Specs)
		{
			const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *DestinationPath, Spec.AssetName, Spec.AssetName);
			UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
			if (!Texture)
			{
				UE_LOG(LogTemp, Error, TEXT("Authored texture import failed: %s"), *ObjectPath);
				bSuccess = false;
				continue;
			}
			Texture->Filter = TF_Nearest;
			Texture->MipGenSettings = TMGS_NoMipmaps;
			Texture->AddressX = TA_Wrap;
			Texture->AddressY = TA_Wrap;
			Texture->CompressionSettings = TC_Default;
			Texture->LODGroup = TEXTUREGROUP_World;
			Texture->SRGB = true;
			Texture->NeverStream = true;
			Texture->PostEditChange();
			bSuccess &= Save(Texture);
		}
		return bSuccess;
	}

	UMaterial* CreateMasterMaterial(UTexture2D* PixelTexture)
	{
		UMaterial* Material = LoadOrCreate<UMaterial>(
			TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/M_LowPoly_Master_Authored"),
			TEXT("M_LowPoly_Master_Authored"));
		if (!Material || !PixelTexture) return nullptr;
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Opaque;
		// Authored pixel palettes must not be recoloured by the warm world light.
		// The material supplies deterministic normal-based bands itself, so Unlit
		// preserves the painted albedo while retaining readable form and shadows.
		Material->SetShadingModel(MSM_Unlit);
		Material->SetUsageByFlag(MATUSAGE_Nanite, true);
		Material->SetUsageByFlag(MATUSAGE_InstancedStaticMeshes, true);
		Material->SetUsageByFlag(MATUSAGE_SkeletalMesh, true);

		auto* UV = CastChecked<UMaterialExpressionTextureCoordinate>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionTextureCoordinate::StaticClass(), -1400, -180));
		auto* TextureResolution = Scalar(Material, TEXT("TexturePixelResolution"), 8.0f, TEXT("01 Pixel Texture"), -1400, -40, 4.0f, 512.0f);
		auto* TextureTiling = Scalar(Material, TEXT("TextureTiling"), 2.0f, TEXT("01 Pixel Texture"), -1400, 40, 0.25f, 32.0f);
		auto* QuantizedUV = CastChecked<UMaterialExpressionCustom>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionCustom::StaticClass(), -1120, -150));
		QuantizedUV->Description = TEXT("Stable object-UV quantization");
		QuantizedUV->OutputType = CMOT_Float2;
		QuantizedUV->Code = TEXT("float R=max(1.0,round(Resolution)); float T=max(0.25,Tiling); float2 P=UV*T; return (floor(P*R)+0.5)/R;");
		Input(QuantizedUV, TEXT("UV"), UV);
		Input(QuantizedUV, TEXT("Resolution"), TextureResolution);
		Input(QuantizedUV, TEXT("Tiling"), TextureTiling);

		auto* BaseTexture = CastChecked<UMaterialExpressionTextureSampleParameter2D>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionTextureSampleParameter2D::StaticClass(), -830, -200));
		BaseTexture->ParameterName = TEXT("BaseTexture");
		BaseTexture->Group = TEXT("01 Pixel Texture");
		BaseTexture->Texture = PixelTexture;
		BaseTexture->SamplerType = SAMPLERTYPE_Color;
		BaseTexture->SamplerSource = SSM_FromTextureAsset;
		BaseTexture->Coordinates.Connect(0, QuantizedUV);

		auto* VertexColor = CastChecked<UMaterialExpressionVertexColor>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionVertexColor::StaticClass(), -820, 35));
		auto* White = CastChecked<UMaterialExpressionConstant3Vector>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionConstant3Vector::StaticClass(), -820, 155));
		White->Constant = FLinearColor::White;
		auto* UseVertexColors = CastChecked<UMaterialExpressionStaticSwitchParameter>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionStaticSwitchParameter::StaticClass(), -580, 90));
		UseVertexColors->ParameterName = TEXT("UseVertexColors");
		UseVertexColors->Group = TEXT("02 Surface Color");
		UseVertexColors->DefaultValue = false;
		UseVertexColors->A.Connect(0, VertexColor);
		UseVertexColors->B.Connect(0, White);

		auto* Normal = CastChecked<UMaterialExpressionVertexNormalWS>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionVertexNormalWS::StaticClass(), -580, 260));
		auto* Tint = Vector(Material, TEXT("BaseTint"), FLinearColor::White, TEXT("02 Surface Color"), -820, 300);
		auto* ShadowTint = Vector(Material, TEXT("ShadowTint"), FLinearColor(0.16f, 0.20f, 0.22f), TEXT("03 Banded Lighting"), -820, 420);
		auto* LightDirection = Vector(Material, TEXT("StylizedLightDirection"), FLinearColor(0.35f, 0.45f, 0.82f), TEXT("03 Banded Lighting"), -820, 540);
		auto* ColorSteps = Scalar(Material, TEXT("ColorSteps"), 8.0f, TEXT("02 Surface Color"), -580, 380, 2.0f, 32.0f);
		auto* Contrast = Scalar(Material, TEXT("Contrast"), 1.05f, TEXT("02 Surface Color"), -580, 460, 0.0f, 2.0f);
		auto* Saturation = Scalar(Material, TEXT("Saturation"), 1.10f, TEXT("02 Surface Color"), -580, 540, 0.0f, 2.0f);
		auto* LightingBands = Scalar(Material, TEXT("LightingBands"), 3.0f, TEXT("03 Banded Lighting"), -580, 620, 2.0f, 4.0f);
		auto* ShadowBrightness = Scalar(Material, TEXT("ShadowBrightness"), 0.42f, TEXT("03 Banded Lighting"), -580, 700, 0.15f, 0.8f);

		auto* Stylize = CastChecked<UMaterialExpressionCustom>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionCustom::StaticClass(), -180, 100));
		Stylize->Description = TEXT("Low-poly color and 2-4 band key-light shaping");
		Stylize->OutputType = CMOT_Float3;
		Stylize->Code = TEXT(
			"float3 C=saturate(TextureColor.rgb*VertexMultiplier.rgb*BaseTint.rgb);"
			"float L=dot(C,float3(0.299,0.587,0.114));"
			"C=lerp(L.xxx,C,Saturation); C=(C-0.5)*Contrast+0.5;"
			"float Steps=max(2.0,round(ColorSteps));"
			"float QL=floor(saturate(dot(C,float3(0.299,0.587,0.114)))*(Steps-1.0)+0.5)/(Steps-1.0);"
			"C=saturate(C*(QL/max(0.0001,dot(C,float3(0.299,0.587,0.114)))));"
			"float NdotL=saturate(dot(normalize(NormalWS),normalize(LightDirection.rgb)));"
			"float Bands=clamp(round(LightingBands),2.0,4.0);"
			"float Band=saturate(floor(NdotL*Bands)/max(1.0,Bands-1.0));"
			"float3 Shade=lerp(ShadowTint.rgb,max(ShadowBrightness,0.001).xxx,Band);"
			"return saturate(C*lerp(ShadowBrightness.xxx,1.0.xxx,Band)+Shade*0.08);");
		Input(Stylize, TEXT("TextureColor"), BaseTexture);
		Input(Stylize, TEXT("VertexMultiplier"), UseVertexColors);
		Input(Stylize, TEXT("BaseTint"), Tint);
		Input(Stylize, TEXT("NormalWS"), Normal);
		Input(Stylize, TEXT("ShadowTint"), ShadowTint);
		Input(Stylize, TEXT("LightDirection"), LightDirection);
		Input(Stylize, TEXT("ColorSteps"), ColorSteps);
		Input(Stylize, TEXT("Contrast"), Contrast);
		Input(Stylize, TEXT("Saturation"), Saturation);
		Input(Stylize, TEXT("LightingBands"), LightingBands);
		Input(Stylize, TEXT("ShadowBrightness"), ShadowBrightness);

		auto* Roughness = Scalar(Material, TEXT("Roughness"), 0.88f, TEXT("04 Surface Response"), 80, 360, 0.0f, 1.0f);
		auto* Specular = Scalar(Material, TEXT("Specular"), 0.08f, TEXT("04 Surface Response"), 80, 440, 0.0f, 0.5f);
		UMaterialEditingLibrary::ConnectMaterialProperty(Stylize, TEXT(""), MP_EmissiveColor);
		Material->PostEditChange();
		UMaterialEditingLibrary::RecompileMaterial(Material);
		return Save(Material) ? Material : nullptr;
	}

	UMaterialExpressionScalarParameter* PPScalar(UMaterial* M, const TCHAR* Name, float Default, FName Group, int32& Y, float Max = 1.0f)
	{
		auto* Result = Scalar(M, Name, Default, Group, -900, Y, 0.0f, Max);
		Y += 72;
		return Result;
	}

	UMaterial* CreatePostProcessMaterial()
	{
		UMaterial* Material = LoadOrCreate<UMaterial>(
			TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/M_PP_ChopItVisualStyle"),
			TEXT("M_PP_ChopItVisualStyle"));
		if (!Material) return nullptr;
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);
		Material->MaterialDomain = MD_PostProcess;
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_Unlit);
		// UE 5.8 reliably exposes PostProcessInput0, scene depth and GBuffer data at
		// Before Bloom. After Tonemapping returned a black input on DX12/SM6 for this
		// generated custom-expression graph.
		Material->BlendableLocation = BL_SceneColorBeforeBloom;

		auto* SceneColor = CastChecked<UMaterialExpressionSceneTexture>(UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionSceneTexture::StaticClass(), -1350, -350));
		SceneColor->SceneTextureId = PPI_PostProcessInput0;
		SceneColor->bFiltered = false;
		auto* SceneDepth = CastChecked<UMaterialExpressionSceneTexture>(UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionSceneTexture::StaticClass(), -1350, -250));
		SceneDepth->SceneTextureId = PPI_SceneDepth;
		auto* CustomDepth = CastChecked<UMaterialExpressionSceneTexture>(UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionSceneTexture::StaticClass(), -1350, -150));
		CustomDepth->SceneTextureId = PPI_CustomDepth;
		auto* CustomStencil = CastChecked<UMaterialExpressionSceneTexture>(UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionSceneTexture::StaticClass(), -1350, -50));
		CustomStencil->SceneTextureId = PPI_CustomStencil;
		auto* WorldNormal = CastChecked<UMaterialExpressionSceneTexture>(UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionSceneTexture::StaticClass(), -1350, 50));
		WorldNormal->SceneTextureId = PPI_WorldNormal;

		int32 Y = -700;
		TMap<FName, UMaterialExpression*> P;
		auto S = [&](const TCHAR* Name, float Default, const TCHAR* Group, float Max = 1.0f)
		{
			UMaterialExpression* E = PPScalar(Material, Name, Default, Group, Y, Max);
			P.Add(Name, E);
			return E;
		};
		S(TEXT("GlobalIntensity"), 1.0f, TEXT("00 Global"));
		S(TEXT("PixelationEnabled"), 1.0f, TEXT("01 Pixelation"));
		S(TEXT("VirtualResolutionHeight"), 360.0f, TEXT("01 Pixelation"), 1080.0f);
		S(TEXT("ColorSteps"), 8.0f, TEXT("02 Color"), 32.0f);
		S(TEXT("Contrast"), 1.20f, TEXT("02 Color"), 2.0f);
		S(TEXT("Saturation"), 0.95f, TEXT("02 Color"), 2.0f);
		P.Add(TEXT("ExposureBias"), Scalar(Material, TEXT("ExposureBias"), -0.65f, TEXT("02 Color"), -900, Y, -3.0f, 3.0f)); Y += 72;
		S(TEXT("ColorGain"), 0.82f, TEXT("02 Color"), 2.0f);
		S(TEXT("AmbientTintStrength"), 0.12f, TEXT("02 Color"));
		P.Add(TEXT("AmbientTint"), Vector(Material, TEXT("AmbientTint"), FLinearColor(0.62f, 0.75f, 0.66f), TEXT("02 Color"), -650, Y)); Y += 72;
		S(TEXT("DitheringEnabled"), 1.0f, TEXT("03 Dithering"));
		S(TEXT("DitherStrength"), 0.22f, TEXT("03 Dithering"));
		S(TEXT("GrainEnabled"), 1.0f, TEXT("04 Grain"));
		S(TEXT("GrainIntensity"), 0.045f, TEXT("04 Grain"), 0.25f);
		S(TEXT("VignetteEnabled"), 1.0f, TEXT("05 Lens"));
		S(TEXT("VignetteIntensity"), 0.22f, TEXT("05 Lens"));
		S(TEXT("ChromaticAberrationEnabled"), 1.0f, TEXT("05 Lens"));
		S(TEXT("ChromaticAberrationPixels"), 0.35f, TEXT("05 Lens"), 3.0f);
		S(TEXT("ScanlinesEnabled"), 0.0f, TEXT("06 Optional"));
		S(TEXT("ScanlineIntensity"), 0.0f, TEXT("06 Optional"), 0.25f);
		S(TEXT("DistortionEnabled"), 0.0f, TEXT("06 Optional"));
		S(TEXT("DistortionIntensity"), 0.0f, TEXT("06 Optional"), 0.05f);
		S(TEXT("DepthFogEnabled"), 0.0f, TEXT("06 Optional"));
		S(TEXT("DepthFogDensity"), 0.0f, TEXT("06 Optional"), 0.001f);
		P.Add(TEXT("DepthFogColor"), Vector(Material, TEXT("DepthFogColor"), FLinearColor(0.13f, 0.34f, 0.32f), TEXT("06 Optional"), -650, Y)); Y += 72;
		S(TEXT("OutlineEnabled"), 1.0f, TEXT("07 Outlines"));
		S(TEXT("OutlineThickness"), 1.15f, TEXT("07 Outlines"), 4.0f);
		S(TEXT("GlobalOutlineEnabled"), 1.0f, TEXT("07 Outlines"));
		S(TEXT("GlobalOutlineThickness"), 1.0f, TEXT("07 Outlines"), 3.0f);
		S(TEXT("GlobalOutlineDepthThreshold"), 0.018f, TEXT("07 Outlines"), 0.2f);
		S(TEXT("GlobalOutlineNormalThreshold"), 0.16f, TEXT("07 Outlines"));
		S(TEXT("GlobalOutlineStrength"), 0.92f, TEXT("07 Outlines"));
		P.Add(TEXT("GlobalOutlineColor"), Vector(Material, TEXT("GlobalOutlineColor"), FLinearColor(0.01f, 0.018f, 0.014f), TEXT("07 Outlines"), -650, Y)); Y += 72;
		P.Add(TEXT("PlayerOutlineColor"), Vector(Material, TEXT("PlayerOutlineColor"), FLinearColor(1.0f, 0.55f, 0.08f), TEXT("07 Outlines"), -650, Y)); Y += 72;
		P.Add(TEXT("EnemyOutlineColor"), Vector(Material, TEXT("EnemyOutlineColor"), FLinearColor(1.0f, 0.14f, 0.06f), TEXT("07 Outlines"), -650, Y)); Y += 72;
		P.Add(TEXT("InteractableOutlineColor"), Vector(Material, TEXT("InteractableOutlineColor"), FLinearColor(0.15f, 0.95f, 0.80f), TEXT("07 Outlines"), -650, Y)); Y += 72;
		S(TEXT("StyleTime"), 0.0f, TEXT("08 Runtime"), 100000.0f);

		auto* Shader = CastChecked<UMaterialExpressionCustom>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionCustom::StaticClass(), 50, -150));
		Shader->Description = TEXT("Dark palette grading, global depth/normal ink and selective gameplay outlines");
		Shader->OutputType = CMOT_Float3;
		Shader->Code = TEXT(R"CHOPIT(
float2 ViewUV = GetViewportUV(Parameters);
float Aspect = View.ViewSizeAndInvSize.x / max(1.0, View.ViewSizeAndInvSize.y);
float2 VirtualSize = float2(max(1.0, VirtualResolutionHeight * Aspect), max(1.0, VirtualResolutionHeight));
float2 DistortedUV = ViewUV;
float Edge = saturate(length((ViewUV - 0.5) * float2(1.0, 0.72)) * 1.55);
if (DistortionEnabled > 0.001)
{
    float Wave = sin((ViewUV.y * 31.0) + StyleTime * 3.2) * DistortionIntensity * Edge;
    DistortedUV.x += Wave * DistortionEnabled;
}
float2 PixelCoord = floor(saturate(DistortedUV) * VirtualSize);
float2 PixelUV = (PixelCoord + 0.5) / VirtualSize;
float2 SampleUV = lerp(saturate(DistortedUV), PixelUV, saturate(PixelationEnabled));
float2 ColorBufferUV = ViewportUVToSceneTextureUV(SampleUV, 14);
// Use the explicit PostProcessInput0 expression for scene color. Naming this
// input InputColor avoids colliding with renderer-owned SceneColor symbols.
float3 C = InputColor.rgb;
C *= exp2(ExposureBias) * ColorGain;

if (ChromaticAberrationEnabled > 0.001 && ChromaticAberrationPixels > 0.001)
{
    float2 Direction = normalize(ViewUV - 0.5 + float2(0.0001, 0.0));
    float2 Shift = Direction * View.ViewSizeAndInvSize.zw * ChromaticAberrationPixels * Edge * Edge;
    float3 CR = SceneTextureLookup(ViewportUVToSceneTextureUV(saturate(SampleUV + Shift), 14), 14, false).rgb;
    float3 CB = SceneTextureLookup(ViewportUVToSceneTextureUV(saturate(SampleUV - Shift), 14), 14, false).rgb;
    C.r = lerp(C.r, CR.r, saturate(ChromaticAberrationEnabled));
    C.b = lerp(C.b, CB.b, saturate(ChromaticAberrationEnabled));
}

float Luma = dot(C, float3(0.299, 0.587, 0.114));
C = lerp(Luma.xxx, C, Saturation);
C = (C - 0.5) * Contrast + 0.5;
C = lerp(C, C * AmbientTint.rgb, saturate(AmbientTintStrength));

int2 BayerP = (int2)PixelCoord & 3;
float Bayer = 0.0;
if (BayerP.y == 0) Bayer = BayerP.x == 0 ? 0.0 : (BayerP.x == 1 ? 8.0 : (BayerP.x == 2 ? 2.0 : 10.0));
else if (BayerP.y == 1) Bayer = BayerP.x == 0 ? 12.0 : (BayerP.x == 1 ? 4.0 : (BayerP.x == 2 ? 14.0 : 6.0));
else if (BayerP.y == 2) Bayer = BayerP.x == 0 ? 3.0 : (BayerP.x == 1 ? 11.0 : (BayerP.x == 2 ? 1.0 : 9.0));
else Bayer = BayerP.x == 0 ? 15.0 : (BayerP.x == 1 ? 7.0 : (BayerP.x == 2 ? 13.0 : 5.0));
Bayer = (Bayer / 15.0) - 0.5;
float Steps = max(2.0, round(ColorSteps));
float DitherOffset = Bayer * DitherStrength * saturate(DitheringEnabled) / Steps;
// Quantize luminance instead of the RGB channels independently.  Channel-wise
// posterization pushes close olive/brown values onto unrelated yellow/pink
// palette corners, which destroys the palette painted into authored textures.
// Scaling the original chroma by the quantized luminance keeps the deliberate
// grass, wood and stone hues while retaining crisp retro value bands.
float PreQuantLuma = max(0.0001, dot(saturate(C), float3(0.299, 0.587, 0.114)));
float QuantizedLuma = floor(saturate(PreQuantLuma + DitherOffset) * (Steps - 1.0) + 0.5) / (Steps - 1.0);
C = saturate(C * (QuantizedLuma / PreQuantLuma));

if (GrainEnabled > 0.001 && GrainIntensity > 0.001)
{
    float Frame = floor(StyleTime * 12.0);
    float Noise = frac(sin(dot(PixelCoord + Frame * float2(13.0, 7.0), float2(12.9898, 78.233))) * 43758.5453) - 0.5;
    C += Noise * GrainIntensity * saturate(GrainEnabled);
}
if (ScanlinesEnabled > 0.001)
{
    float Scan = (fmod(PixelCoord.y, 2.0) < 1.0 ? -1.0 : 0.25) * ScanlineIntensity;
    C += Scan * saturate(ScanlinesEnabled);
}
if (DepthFogEnabled > 0.001 && DepthFogDensity > 0.0)
{
    float Fog = 1.0 - exp(-max(0.0, SceneDepth.r - 300.0) * DepthFogDensity);
    C = lerp(C, DepthFogColor.rgb, saturate(Fog * DepthFogEnabled));
}
if (VignetteEnabled > 0.001)
{
    float V = smoothstep(0.38, 1.0, Edge);
    C *= 1.0 - V * VignetteIntensity * saturate(VignetteEnabled);
}

// Thin global ink line. Relative linear depth keeps the threshold stable with
// distance and avoids tracing the high-frequency authored texture art. World
// normals are kept as an explicit material dependency for a future pre-tonemap
// pass, but UE 5.8 does not preserve them reliably at this blendable location.
if (GlobalOutlineEnabled > 0.001)
{
    float2 OutlinePixel = View.ViewSizeAndInvSize.zw * max(0.5, GlobalOutlineThickness);
    float CenterDepth = SceneTextureLookup(ViewportUVToSceneTextureUV(ViewUV, 1), 1, false).r;
    float2 EdgeUV[4] =
    {
        saturate(ViewUV + float2(OutlinePixel.x, 0.0)),
        saturate(ViewUV - float2(OutlinePixel.x, 0.0)),
        saturate(ViewUV + float2(0.0, OutlinePixel.y)),
        saturate(ViewUV - float2(0.0, OutlinePixel.y))
    };
    float DepthDelta = 0.0;
    [unroll] for (int EdgeIndex = 0; EdgeIndex < 4; ++EdgeIndex)
    {
        float NeighbourDepth = SceneTextureLookup(ViewportUVToSceneTextureUV(EdgeUV[EdgeIndex], 1), 1, false).r;
        float RelativeDepth = abs(NeighbourDepth - CenterDepth) / max(100.0, abs(CenterDepth));
        DepthDelta = max(DepthDelta, RelativeDepth);
    }
    float DepthInk = smoothstep(GlobalOutlineDepthThreshold * 0.65, GlobalOutlineDepthThreshold, DepthDelta);
    float GlobalInk = saturate(DepthInk * GlobalOutlineStrength * GlobalOutlineEnabled);
    C = lerp(C, GlobalOutlineColor.rgb, GlobalInk);
}

if (OutlineEnabled > 0.001)
{
    float2 PixelOffset = View.ViewSizeAndInvSize.zw * max(0.5, OutlineThickness);
    float2 NUV[4] = { saturate(ViewUV + float2(PixelOffset.x,0)), saturate(ViewUV - float2(PixelOffset.x,0)), saturate(ViewUV + float2(0,PixelOffset.y)), saturate(ViewUV - float2(0,PixelOffset.y)) };
    float CenterRaw = CustomStencil.r;
    float CenterStencil = CenterRaw <= 0.0 ? 0.0 : (CenterRaw < 0.999 ? round(CenterRaw * 255.0) : round(CenterRaw));
	// Keep a very light interior accent so outlined gameplay objects remain
	// legible against equally saturated backgrounds; the strong color below is
	// still restricted to the actual silhouette.
	if (CenterStencil > 0.5)
	{
		float3 InteriorColor = CenterStencil < 1.5 ? PlayerOutlineColor.rgb : (CenterStencil < 2.5 ? EnemyOutlineColor.rgb : InteractableOutlineColor.rgb);
		C = lerp(C, InteriorColor, 0.12 * saturate(OutlineEnabled));
	}
    float FoundStencil = 0.0;
    float2 FoundUV = ViewUV;
    if (CenterStencil < 0.5)
    {
        [unroll] for (int I = 0; I < 4; ++I)
        {
            float RawS = SceneTextureLookup(ViewportUVToSceneTextureUV(NUV[I], 25), 25, false).r;
            float S = RawS <= 0.0 ? 0.0 : (RawS < 0.999 ? round(RawS * 255.0) : round(RawS));
            if (S > 0.5 && FoundStencil < 0.5) { FoundStencil = S; FoundUV = NUV[I]; }
        }
    }
	else
	{
		[unroll] for (int I = 0; I < 4; ++I)
		{
			float RawS = SceneTextureLookup(ViewportUVToSceneTextureUV(NUV[I], 25), 25, false).r;
			float S = RawS <= 0.0 ? 0.0 : (RawS < 0.999 ? round(RawS * 255.0) : round(RawS));
			if (S < 0.5) { FoundStencil = CenterStencil; FoundUV = ViewUV; }
		}
	}
		    // The stencil is already selective.  Keeping the edge test independent of
		    // depth avoids platform-dependent CustomDepth/SceneDepth unit mismatches
		    // that could make the outline disappear entirely after tonemapping.
		    // An fwidth fallback also preserves a crisp one-pixel silhouette on RHIs
		    // where neighbour reads of CustomStencil are unavailable in this pass.
		    float DerivativeEdge = saturate(fwidth(CenterRaw) * 255.0);
		    if (FoundStencil < 0.5 && CenterStencil > 0.5 && DerivativeEdge > 0.001)
		    {
		        FoundStencil = CenterStencil;
		    }
		    if (FoundStencil > 0.5)
		    {
		        float3 OutlineColor = FoundStencil < 1.5 ? PlayerOutlineColor.rgb : (FoundStencil < 2.5 ? EnemyOutlineColor.rgb : InteractableOutlineColor.rgb);
		        C = OutlineColor;
		    }
		}
float3 Original = InputColor.rgb + (CustomDepth.r + CustomStencil.r + WorldNormal.r) * 0.0;
return lerp(Original, saturate(C), saturate(GlobalIntensity));
)CHOPIT");
		Input(Shader, TEXT("InputColor"), SceneColor);
		Input(Shader, TEXT("SceneDepth"), SceneDepth);
		Input(Shader, TEXT("CustomDepth"), CustomDepth);
		Input(Shader, TEXT("CustomStencil"), CustomStencil);
		Input(Shader, TEXT("WorldNormal"), WorldNormal);
		for (const TPair<FName, UMaterialExpression*>& Pair : P) Input(Shader, *Pair.Key.ToString(), Pair.Value);
		UMaterialEditingLibrary::ConnectMaterialProperty(Shader, TEXT(""), MP_EmissiveColor);
		Material->PostEditChange();
		const TArray<FString> Errors = UMaterialEditingLibrary::RecompileMaterial(Material);
		for (const FString& Error : Errors) UE_LOG(LogTemp, Error, TEXT("Visual style material: %s"), *Error);
		return Errors.Num() == 0 && Save(Material) ? Material : nullptr;
	}

	bool CreateInstances(UMaterial* Parent)
	{
		struct FSpec { const TCHAR* Name; FLinearColor Tint; FLinearColor Shadow; float Steps; float Saturation; float Roughness; };
		const FSpec Specs[] =
		{
			{ TEXT("MI_LowPoly_Forest"), FLinearColor(0.12f, 0.44f, 0.16f), FLinearColor(0.035f, 0.10f, 0.08f), 10.0f, 1.08f, 0.92f },
			{ TEXT("MI_LowPoly_Industrial"), FLinearColor(0.42f, 0.30f, 0.18f), FLinearColor(0.11f, 0.09f, 0.075f), 7.0f, 0.82f, 0.96f },
			{ TEXT("MI_LowPoly_Magic"), FLinearColor(0.38f, 0.08f, 0.58f), FLinearColor(0.07f, 0.025f, 0.13f), 9.0f, 1.18f, 0.80f }
		};
		for (const FSpec& Spec : Specs)
		{
			const FString Package = FString::Printf(TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Instances/%s"), Spec.Name);
			UMaterialInstanceConstant* MI = LoadOrCreate<UMaterialInstanceConstant>(Package, Spec.Name);
			if (!MI) return false;
			MI->SetParentEditorOnly(Parent);
			MI->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("BaseTint")), Spec.Tint);
			MI->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("ShadowTint")), Spec.Shadow);
			MI->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("ColorSteps")), Spec.Steps);
			MI->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Saturation")), Spec.Saturation);
			MI->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Roughness")), Spec.Roughness);
			MI->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("LightingBands")), 3.0f);
			MI->PostEditChange();
			if (!Save(MI)) return false;
		}
		return true;
	}

	bool CreateOutlineOverlayAssets()
	{
		UMaterial* Material = LoadOrCreate<UMaterial>(
			TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Outline/M_OutlineOverlay"),
			TEXT("M_OutlineOverlay"));
		if (!Material) return false;
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Masked;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->OpacityMaskClipValue = 0.5f;
		Material->SetUsageByFlag(MATUSAGE_Nanite, true);
		Material->SetUsageByFlag(MATUSAGE_InstancedStaticMeshes, true);
		Material->SetUsageByFlag(MATUSAGE_SkeletalMesh, true);

		auto* Color = Vector(Material, TEXT("OutlineColor"), FLinearColor(0.15f, 0.95f, 0.80f), TEXT("Outline"), -520, -120);
		auto* Width = Scalar(Material, TEXT("OutlineWidth"), 3.5f, TEXT("Outline"), -520, 20, 0.0f, 12.0f);
		auto* Normal = CastChecked<UMaterialExpressionVertexNormalWS>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionVertexNormalWS::StaticClass(), -520, 150));
		auto* Expand = CastChecked<UMaterialExpressionMultiply>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionMultiply::StaticClass(), -240, 120));
		Expand->A.Connect(0, Normal);
		Expand->B.Connect(0, Width);
		auto* Sign = CastChecked<UMaterialExpressionTwoSidedSign>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionTwoSidedSign::StaticClass(), -520, 300));
		auto* BackFaceOnly = CastChecked<UMaterialExpressionCustom>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, UMaterialExpressionCustom::StaticClass(), -240, 300));
		BackFaceOnly->Description = TEXT("Inverted hull: render expanded back faces only");
		BackFaceOnly->OutputType = CMOT_Float1;
		BackFaceOnly->Code = TEXT("return TwoSided < 0.0 ? 1.0 : 0.0;");
		Input(BackFaceOnly, TEXT("TwoSided"), Sign);
		UMaterialEditingLibrary::ConnectMaterialProperty(Color, TEXT(""), MP_EmissiveColor);
		UMaterialEditingLibrary::ConnectMaterialProperty(Expand, TEXT(""), MP_WorldPositionOffset);
		UMaterialEditingLibrary::ConnectMaterialProperty(BackFaceOnly, TEXT(""), MP_OpacityMask);
		Material->PostEditChange();
		const TArray<FString> Errors = UMaterialEditingLibrary::RecompileMaterial(Material);
		for (const FString& Error : Errors) UE_LOG(LogTemp, Error, TEXT("Outline overlay material: %s"), *Error);
		if (Errors.Num() > 0 || !Save(Material)) return false;

		struct FOutlineSpec { const TCHAR* Name; FLinearColor Color; float Width; };
		const FOutlineSpec Specs[] =
		{
			{ TEXT("MI_Outline_Global"), FLinearColor(0.006f, 0.012f, 0.009f), 1.25f },
			{ TEXT("MI_Outline_Player"), FLinearColor(1.0f, 0.48f, 0.025f), 3.5f },
			{ TEXT("MI_Outline_Enemy"), FLinearColor(1.0f, 0.035f, 0.015f), 3.5f },
			{ TEXT("MI_Outline_Interactable"), FLinearColor(0.01f, 1.0f, 0.82f), 3.5f }
		};
		for (const FOutlineSpec& Spec : Specs)
		{
			const FString Package = FString::Printf(TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Outline/%s"), Spec.Name);
			UMaterialInstanceConstant* MI = LoadOrCreate<UMaterialInstanceConstant>(Package, Spec.Name);
			if (!MI) return false;
			MI->SetParentEditorOnly(Material);
			MI->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("OutlineColor")), Spec.Color);
			MI->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("OutlineWidth")), Spec.Width);
			MI->PostEditChange();
			if (!Save(MI)) return false;
		}
		return true;
	}

	bool CreatePresets()
	{
		auto Make = [](const TCHAR* Name, EChopItVisualPreset Kind) -> UChopItVisualStylePreset*
		{
			const FString Package = FString::Printf(TEXT("/Game/ChopIt/Presentation/VisualStyle/Presets/%s"), Name);
			UChopItVisualStylePreset* Preset = LoadOrCreate<UChopItVisualStylePreset>(Package, Name);
			if (Preset) Preset->Preset = Kind;
			return Preset;
		};

		UChopItVisualStylePreset* Megabonk = Make(TEXT("DA_VisualPreset_Megabonk"), EChopItVisualPreset::Megabonk);
		UChopItVisualStylePreset* Machine = Make(TEXT("DA_VisualPreset_MachineParty"), EChopItVisualPreset::MachineParty);
		UChopItVisualStylePreset* Hybrid = Make(TEXT("DA_VisualPreset_Hybrid"), EChopItVisualPreset::Hybrid);
		if (!Megabonk || !Machine || !Hybrid) return false;

		Megabonk->ColorSteps = 8.0f; Megabonk->Contrast = 1.24f; Megabonk->Saturation = 1.02f;
		Megabonk->ExposureBias = -0.78f; Megabonk->ColorGain = 0.78f;
		Megabonk->bChromaticAberration = false;
		Megabonk->bOutlines = true;
		Megabonk->bGlobalOutline = true; Megabonk->GlobalOutlineColor = FLinearColor(0.008f, 0.015f, 0.012f);
		Megabonk->GlobalOutlineThickness = 1.0f; Megabonk->GlobalOutlineDepthThreshold = 0.016f;
		Megabonk->GlobalOutlineNormalThreshold = 0.14f; Megabonk->GlobalOutlineStrength = 0.96f;
		Megabonk->AmbientTint = FLinearColor(0.56f, 0.72f, 0.64f); Megabonk->AmbientTintStrength = 0.14f;
		Megabonk->VirtualResolutionHeight = 288.0f; Megabonk->DitherStrength = 0.16f; Megabonk->GrainIntensity = 0.012f;
		Megabonk->VignetteIntensity = 0.13f; Megabonk->ChromaticAberrationPixels = 0.10f; Megabonk->OutlineThickness = 2.75f;
		Megabonk->FogColor = FLinearColor(0.025f, 0.18f, 0.22f); Megabonk->FogDensity = 0.039f;
		Megabonk->FogStartDistance = 220.0f; Megabonk->FogMaxOpacity = 0.90f; Megabonk->FogHeightFalloff = 0.17f;
		Megabonk->BloomIntensity = 0.07f;

		Machine->ColorSteps = 4.0f; Machine->Contrast = 1.32f; Machine->Saturation = 0.62f;
		Machine->ExposureBias = -0.20f; Machine->ColorGain = 0.94f;
		Machine->bChromaticAberration = false;
		Machine->bOutlines = true;
		Machine->bGlobalOutline = true; Machine->GlobalOutlineColor = FLinearColor(0.025f, 0.018f, 0.012f);
		Machine->GlobalOutlineThickness = 1.0f; Machine->GlobalOutlineDepthThreshold = 0.022f;
		Machine->GlobalOutlineNormalThreshold = 0.20f; Machine->GlobalOutlineStrength = 0.82f;
		Machine->AmbientTint = FLinearColor(0.60f, 0.45f, 0.29f); Machine->AmbientTintStrength = 0.34f;
		Machine->VirtualResolutionHeight = 200.0f; Machine->DitherStrength = 0.72f; Machine->GrainIntensity = 0.13f;
		Machine->VignetteIntensity = 0.50f; Machine->ChromaticAberrationPixels = 0.90f; Machine->OutlineThickness = 2.20f;
		Machine->FogColor = FLinearColor(0.36f, 0.29f, 0.22f); Machine->FogDensity = 0.026f;
		Machine->FogStartDistance = 260.0f; Machine->FogMaxOpacity = 0.82f; Machine->FogHeightFalloff = 0.22f;
		Machine->BloomIntensity = 0.08f;

		Hybrid->ColorSteps = 6.0f; Hybrid->Contrast = 1.20f; Hybrid->Saturation = 0.95f;
		Hybrid->ExposureBias = -0.65f; Hybrid->ColorGain = 0.82f;
		Hybrid->bChromaticAberration = false;
		Hybrid->bOutlines = true;
		Hybrid->bGlobalOutline = true; Hybrid->GlobalOutlineColor = FLinearColor(0.01f, 0.018f, 0.014f);
		Hybrid->GlobalOutlineThickness = 1.0f; Hybrid->GlobalOutlineDepthThreshold = 0.018f;
		Hybrid->GlobalOutlineNormalThreshold = 0.16f; Hybrid->GlobalOutlineStrength = 0.92f;
		Hybrid->AmbientTint = FLinearColor(0.62f, 0.75f, 0.66f); Hybrid->AmbientTintStrength = 0.12f;
		Hybrid->VirtualResolutionHeight = 240.0f; Hybrid->DitherStrength = 0.34f; Hybrid->GrainIntensity = 0.050f;
		Hybrid->VignetteIntensity = 0.26f; Hybrid->ChromaticAberrationPixels = 0.35f; Hybrid->OutlineThickness = 2.50f;
		Hybrid->FogColor = FLinearColor(0.035f, 0.20f, 0.23f); Hybrid->FogDensity = 0.035f;
		Hybrid->FogStartDistance = 250.0f; Hybrid->FogMaxOpacity = 0.88f; Hybrid->FogHeightFalloff = 0.18f;
		Hybrid->BloomIntensity = 0.08f;

		return Save(Megabonk) && Save(Machine) && Save(Hybrid);
	}
}

bool UChopItBootstrapCommandlet::CreateVisualStyleAssets() const
{
	UTexture2D* PixelTexture = ChopItVisualBootstrap::CreatePixelTexture();
	const bool bAuthoredTexturesReady = ChopItVisualBootstrap::ImportAuthoredSurfaceTextures();
	// Once production instances reference the master, UE may root its expressions while
	// resolving instance inheritance. Reuse the authored master on repeat runs; create it
	// only for a fresh project. The independently referenced PP material remains rebuildable.
	UMaterial* Master = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/M_LowPoly_Master_Authored.M_LowPoly_Master_Authored"));
	if (!Master) Master = ChopItVisualBootstrap::CreateMasterMaterial(PixelTexture);
	else
	{
		Master->SetUsageByFlag(MATUSAGE_Nanite, true);
		Master->SetUsageByFlag(MATUSAGE_InstancedStaticMeshes, true);
		Master->SetUsageByFlag(MATUSAGE_SkeletalMesh, true);
		Master->PostEditChange();
		ChopItVisualBootstrap::Save(Master);
	}
	UMaterial* PostProcess = ChopItVisualBootstrap::CreatePostProcessMaterial();
	return PixelTexture && bAuthoredTexturesReady && Master && PostProcess
		&& ChopItVisualBootstrap::CreateInstances(Master)
		&& ChopItVisualBootstrap::CreateOutlineOverlayAssets()
		&& ChopItVisualBootstrap::CreatePresets()
		&& CreateBlockoutMaterials()
		&& RebuildVisualStyleDemoMap();
}

bool UChopItBootstrapCommandlet::RebuildVisualStyleDemoMap() const
{
	constexpr TCHAR DemoMap[] = TEXT("/Game/ChopIt/World/Maps/L_VisualStyleDemo");
	UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
	if (!World) return false;
	World->GetWorldSettings()->DefaultGameMode = AChopItGameMode::StaticClass();

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	UMaterialInterface* Forest = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Instances/MI_LowPoly_Forest.MI_LowPoly_Forest"));
	UMaterialInterface* Industrial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Instances/MI_LowPoly_Industrial.MI_LowPoly_Industrial"));
	UMaterialInterface* Magic = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Instances/MI_LowPoly_Magic.MI_LowPoly_Magic"));
	if (!Cube || !Sphere || !Cone || !Forest || !Industrial || !Magic) return false;

	auto SpawnMesh = [&](const TCHAR* Name, UStaticMesh* Mesh, FVector Location, FVector Scale, UMaterialInterface* Material)
	{
		FActorSpawnParameters Params; Params.Name = Name;
		AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Params);
		if (!Actor) return static_cast<AStaticMeshActor*>(nullptr);
		Actor->SetActorLabel(Name);
		Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		Actor->GetStaticMeshComponent()->SetMaterial(0, Material);
		Actor->SetActorScale3D(Scale);
		return Actor;
	};
	SpawnMesh(TEXT("DemoGround"), Cube, FVector(0, 0, -60), FVector(18, 18, 0.5), Forest);
	for (int32 X = -3; X <= 3; ++X)
	{
		SpawnMesh(*FString::Printf(TEXT("ForestShape_%d"), X), X % 2 ? Cone : Sphere, FVector(X * 260.0f, 320.0f, 90.0f), FVector(1.1f), Forest);
		SpawnMesh(*FString::Printf(TEXT("IndustrialShape_%d"), X), X % 2 ? Cube : Cone, FVector(X * 260.0f, 0.0f, 90.0f), FVector(1.1f), Industrial);
		SpawnMesh(*FString::Printf(TEXT("MagicShape_%d"), X), X % 2 ? Sphere : Cube, FVector(X * 260.0f, -320.0f, 90.0f), FVector(1.1f), Magic);
	}

	FActorSpawnParameters Params;
	Params.Name = TEXT("PlayerStart");
	World->SpawnActor<APlayerStart>(FVector(0, -650, 120), FRotator(0, 25, 0), Params);
	Params.Name = TEXT("DemoEnemy_Stencil2");
	AChopItEnemyCharacter* DemoEnemy = World->SpawnActor<AChopItEnemyCharacter>(FVector(-260, -360, 70), FRotator::ZeroRotator, Params);
	if (DemoEnemy) DemoEnemy->SetActorLabel(TEXT("Enemy outline - stencil 2"));
	Params.Name = TEXT("DemoInteractable_Stencil3");
	AChopItShopTerminal* DemoInteractable = World->SpawnActor<AChopItShopTerminal>(FVector(260, -350, 0), FRotator::ZeroRotator, Params);
	if (DemoInteractable) DemoInteractable->SetActorLabel(TEXT("Interactable outline - stencil 3"));
	Params.Name = TEXT("DirectionalLight");
	ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-52, -28, 0), Params);
	Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	Sun->GetLightComponent()->SetIntensity(3.2f);
	Sun->GetLightComponent()->SetLightColor(FLinearColor(0.82f, 0.88f, 0.74f));
	Params.Name = TEXT("SkyLight");
	ASkyLight* Sky = World->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	Sky->GetLightComponent()->SetIntensity(0.62f);
	Sky->GetLightComponent()->SetRealTimeCapture(true);
	Params.Name = TEXT("SkyAtmosphere");
	World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	Params.Name = TEXT("HeightFog_ExteriorTeal");
	AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(FVector(0, 0, -80), FRotator::ZeroRotator, Params);
	Fog->GetComponent()->SetFogDensity(0.035f);
	Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(0.035f, 0.20f, 0.23f));
	Fog->GetComponent()->SetFogHeightFalloff(0.18f);
	Fog->GetComponent()->SetFogMaxOpacity(0.88f);
	Fog->GetComponent()->SetStartDistance(250.0f);
	Params.Name = TEXT("ChopIt_GlobalPostProcess_Reference");
	APostProcessVolume* PPV = World->SpawnActor<APostProcessVolume>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	PPV->SetActorLabel(TEXT("ChopIt Global Post Process (runtime component supplies blendable)"));
	PPV->bUnbound = true;
	PPV->Priority = 40.0f;
	PPV->BlendWeight = 1.0f;
	PPV->Tags.Add(TEXT("ChopItVisualStylePreview"));
	PPV->Settings.WeightedBlendables.Array.Reset();
	PPV->Settings.bOverride_AutoExposureMethod = false;
	PPV->Settings.bOverride_AutoExposureBias = true;
	PPV->Settings.AutoExposureBias = -0.65f;
	PPV->Settings.bOverride_BloomIntensity = true;
	PPV->Settings.BloomIntensity = 0.08f;
	PPV->Settings.bOverride_AmbientOcclusionIntensity = true;
	PPV->Settings.AmbientOcclusionIntensity = 0.48f;
	PPV->Settings.bOverride_MotionBlurAmount = true;
	PPV->Settings.MotionBlurAmount = 0.0f;
	PPV->Settings.bOverride_SceneFringeIntensity = true;
	PPV->Settings.SceneFringeIntensity = 0.0f;
	if (UMaterialInterface* GlobalOutline = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Outline/MI_Outline_Global.MI_Outline_Global")))
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TArray<UMeshComponent*> Meshes;
			It->GetComponents<UMeshComponent>(Meshes);
			for (UMeshComponent* Mesh : Meshes)
			{
				if (IsValid(Mesh) && Mesh->IsVisible()) Mesh->SetOverlayMaterial(GlobalOutline);
			}
		}
	}

	return UEditorLoadingAndSavingUtils::SaveMap(World, DemoMap);
}
