#include "Terrain/ChopItTerrainActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/DirectionalLight.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RenderingThread.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "FileHelpers.h"
#include "Tests/AutomationEditorCommon.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainVisualTest, "ChopIt.Terrain.Geometry.LevelRender", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)
bool FChopItTerrainVisualTest::RunTest(const FString& Parameters)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	if (!TestNotNull(TEXT("Editor test level"), World)) return false;
	AChopItTerrainActor* Terrain = World->SpawnActor<AChopItTerrainActor>();
	if (!TestNotNull(TEXT("Terrain inside editor level"), Terrain)) return false;
	Terrain->SetActorLabel(TEXT("Phase 2 Forest Terrain"));
	Terrain->MeshSettings.PolygonDetail = 45;
	Terrain->MeshSettings.CellsPerChunk = 16;
	FString Error;
	if (!TestTrue(TEXT("Generate level terrain"), Terrain->RebuildTerrain(Error))) return false;
	if (!TestNotNull(TEXT("Default ground material"), Terrain->TerrainMaterial.Get())) return false;
	AddInfo(FString::Printf(TEXT("Ground material: %s"), *Terrain->TerrainMaterial->GetPathName()));
	AddInfo(FString::Printf(TEXT("Base material: %s"), *Terrain->TerrainMaterial->GetMaterial()->GetPathName()));
	if (UMaterialInstance* Instance = Cast<UMaterialInstance>(Terrain->TerrainMaterial))
	{
		AddInfo(FString::Printf(TEXT("Parent: %s"), *GetPathNameSafe(Instance->Parent)));
	}
	for (UDynamicMeshComponent* Chunk : Terrain->GetTerrainChunks())
	{
		TestTrue(TEXT("Ground material assigned to chunk"), Chunk->GetMaterial(0) == Terrain->TerrainMaterial);
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(FVector::ZeroVector, Index == 0 ? FRotator(-25, -35, 0) : FRotator(-65, 140, 0));
		Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		Light->GetLightComponent()->SetIntensity(Index == 0 ? 6.0f : 1.5f);
	}
	AActor* Camera = World->SpawnActor<AActor>();
	USceneCaptureComponent2D* Capture = NewObject<USceneCaptureComponent2D>(Camera);
	Camera->SetRootComponent(Capture);
	Capture->RegisterComponent();
	UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(Capture);
	Target->InitAutoFormat(1024, 768);
	Capture->TextureTarget = Target;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;
	Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
	Capture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
	Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
	Capture->PostProcessSettings.AutoExposureBias = 0.0f;
	Capture->SetWorldLocation(FVector(16500, -20500, 16000));
	Capture->SetWorldRotation((FVector(0, 0, 100) - Capture->GetComponentLocation()).Rotation());
	Capture->FOVAngle = 55.0f;
	// A synchronous capture cannot wait for the editor's on-demand material compilation tick.
	Terrain->TerrainMaterial->GetMaterial()->ForceRecompileForRendering();
	Terrain->TerrainMaterial->ForceRecompileForRendering();
	FAssetCompilingManager::Get().FinishAllCompilation();
	if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("Terrain")), true);
	for (EChopItTerrainShading Shading : { EChopItTerrainShading::Smooth, EChopItTerrainShading::Flat })
	{
		Terrain->MeshSettings.Shading = Shading;
		if (!Terrain->RebuildTerrain(Error)) return false;
		for (UDynamicMeshComponent* Chunk : Terrain->GetTerrainChunks()) Chunk->MarkRenderStateDirty();
		for (int32 Frame = 0; Frame < 4; ++Frame)
		{
			++GFrameCounter;
			World->SendAllEndOfFrameUpdates();
			FlushRenderingCommands();
			Capture->CaptureScene();
			FlushRenderingCommands();
		}
		TArray<FColor> Pixels;
		if (!TestTrue(TEXT("Read level capture"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels))) return false;
		int32 VisiblePixels = 0;
		for (FColor& Pixel : Pixels)
		{
			if (Pixel.G > 15 && Pixel.G > Pixel.R * 1.2f && Pixel.G > Pixel.B * 1.2f) ++VisiblePixels;
			Pixel.A = 255;
		}
		TestTrue(TEXT("Rendered green terrain covers a meaningful part of the image"), VisiblePixels > Pixels.Num() / 10);
		TArray64<uint8> PNG;
		FImageUtils::PNGCompressImageArray(1024, 768, Pixels, PNG);
		const FString Filename = FPaths::ProjectSavedDir() / TEXT("Terrain") / (Shading == EChopItTerrainShading::Smooth ? TEXT("Phase2_Smooth.png") : TEXT("Phase2_Flat.png"));
		TestTrue(TEXT("Save visual evidence"), FFileHelper::SaveArrayToFile(PNG, *Filename));
	}
	Camera->Destroy();
	const FString MapDirectory = FPaths::ProjectContentDir() / TEXT("ChopIt/Developer/Terrain");
	IFileManager::Get().MakeDirectory(*MapDirectory, true);
	TestTrue(TEXT("Save isolated inspection level"), FEditorFileUtils::SaveLevel(World->PersistentLevel, MapDirectory / TEXT("L_Terrain_Phase2.umap")));
	return true;
}
#endif
