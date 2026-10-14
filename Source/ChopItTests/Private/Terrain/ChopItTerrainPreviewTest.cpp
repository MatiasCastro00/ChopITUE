#include "Terrain/ChopItTerrainPreview.h"
#include "Terrain/SChopItTerrainPreviewViewport.h"
#include "Terrain/ChopItTerrainGenerator.h"
#include "Components/DynamicMeshComponent.h"
#include "UDynamicMesh.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Materials/Material.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SViewport.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
bool FinishPreview(FChopItTerrainPreview& Preview, double Now)
{
	const double Deadline = FPlatformTime::Seconds() + 15;
	while (Preview.IsBusy() && FPlatformTime::Seconds() < Deadline)
	{
		Preview.Tick(Now);
		FPlatformProcess::Sleep(0.005f);
	}
	return !Preview.IsBusy();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainPreviewDataTest, "ChopIt.Terrain.Preview.DataAndCamera",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainPreviewDataTest::RunTest(const FString& Parameters)
{
	FChopItTerrainPreview Preview;
	FChopItTerrainSettings Terrain;
	Terrain.SamplesX = 65; Terrain.SamplesY = 49;
	FChopItTerrainMeshSettings Mesh;
	Preview.Request(Terrain, Mesh, 10, 0.0);
	Preview.Tick(0.1);
	TestEqual(TEXT("Debounce leaves scene untouched"), Preview.GetComponents().Num(), 0);
	Terrain.Seed = 412;
	Preview.Request(Terrain, Mesh, 10, 0.2);
	Preview.Tick(0.3);
	TestEqual(TEXT("New edit restarts debounce"), Preview.GetComponents().Num(), 0);
	if (!TestTrue(TEXT("Preview worker completes"), FinishPreview(Preview, 0.5))) return false;
	if (!TestNotNull(TEXT("Preview has height field"), Preview.GetHeightField())) return false;
	TestTrue(TEXT("Preview reports calculated water coverage"), Preview.GetStatus().ToString().Contains(TEXT("Approx. submerged area")));
	FChopItHeightField Expected;
	FString Error;
	FChopItTerrainGenerator::Generate(Terrain, Expected, Error);
	TestTrue(TEXT("Preview uses exact definitive heightmap"), Preview.GetHeightField()->GetHeights() == Expected.GetHeights());
	const float* CachedHeights = Preview.GetHeightField()->GetHeights().GetData();
	const float* CachedMask = Preview.GetHeightField()->GetMaps().Clearing.GetData();
	UDynamicMeshComponent* First = Preview.GetComponents()[0];
	const int32 LowTriangles = First->GetDynamicMesh()->GetMeshRef().TriangleCount();
	Mesh.Shading = EChopItTerrainShading::Flat;
	Preview.Request(Terrain, Mesh, 70, 1.0);
	if (!TestTrue(TEXT("Detail and shading rebuild complete"), FinishPreview(Preview, 1.3))) return false;
	TestTrue(TEXT("Detail reuses exact source allocation"), CachedHeights == Preview.GetHeightField()->GetHeights().GetData());
	TestTrue(TEXT("Polygon detail preserves the spatial maps"), CachedMask == Preview.GetHeightField()->GetMaps().Clearing.GetData());
	TestTrue(TEXT("Preview component reused"), First == Preview.GetComponents()[0]);
	TestTrue(TEXT("Preview detail changes geometry"), First->GetDynamicMesh()->GetMeshRef().TriangleCount() > LowTriangles);
	for (UDynamicMeshComponent* Component : Preview.GetComponents())
	{
		TestTrue(TEXT("Preview collision disabled"), Component->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Preview has no physics body"), Component->IsPhysicsStateCreated());
		for (int32 Vertex : Component->GetDynamicMesh()->GetMeshRef().VertexIndicesItr())
		{
			const FVector3d P = Component->GetDynamicMesh()->GetMeshRef().GetVertex(Vertex);
			float Height = 0;
			Expected.SampleLocalHeight(FVector2D(P.X, P.Y), Height);
			if (!TestTrue(TEXT("Every preview vertex samples the definitive heightmap"), FMath::Abs(P.Z - Height) < 0.001)) return false;
		}
	}
	Terrain.Seed = 13;
	Preview.Request(Terrain, Mesh, 20, 2);
	Preview.Tick(2.3);
	Terrain.Seed = 99;
	Preview.Request(Terrain, Mesh, 20, 2.31);
	if (!TestTrue(TEXT("Latest edit completes after in-flight work"), FinishPreview(Preview, 2.7))) return false;
	FChopItTerrainGenerator::Generate(Terrain, Expected, Error);
	TestTrue(TEXT("In-flight stale result cannot win"), Preview.GetHeightField()->GetHeights() == Expected.GetHeights());
	const uint64 Revision = Preview.GetPublishedRevision();
	Terrain.SamplesX = 0;
	Preview.Request(Terrain, Mesh, 20, 3);
	FinishPreview(Preview, 3.3);
	TestEqual(TEXT("Invalid input preserves visible preview"), Preview.GetPublishedRevision(), Revision);
	TestTrue(TEXT("Invalid input reports diagnosis"), Preview.GetStatus().ToString().Contains(TEXT("Heightmap sample counts")));

	FChopItTerrainPreviewViewportClient Camera(Preview.GetScene(), nullptr);
	Camera.FrameTerrain(Preview.GetBounds());
	const FVector OriginalTarget = Camera.GetOrbitTarget();
	const FRotator OriginalRotation = Camera.GetViewRotation();
	const double OriginalDistance = Camera.GetOrbitDistance();
	Camera.InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton, IE_Pressed, 1));
	Camera.InputAxis(FInputKeyEventArgs::CreateSimulated(EKeys::MouseX, IE_Axis, 30));
	Camera.InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton, IE_Released, 0));
	TestFalse(TEXT("Mouse drag rotates camera"), Camera.GetViewRotation().Equals(OriginalRotation));
	TestTrue(TEXT("Orbit keeps target"), Camera.GetOrbitTarget().Equals(OriginalTarget));
	Camera.InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MouseScrollUp, IE_Pressed, 1));
	TestTrue(TEXT("Mouse wheel zooms in"), Camera.GetOrbitDistance() < OriginalDistance);
	Camera.InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MiddleMouseButton, IE_Pressed, 1));
	Camera.InputAxis(FInputKeyEventArgs::CreateSimulated(EKeys::MouseY, IE_Axis, 20));
	Camera.InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MiddleMouseButton, IE_Released, 0));
	TestFalse(TEXT("Middle drag pans target"), Camera.GetOrbitTarget().Equals(OriginalTarget));
	Camera.FrameTerrain(Preview.GetBounds(), true);
	TestTrue(TEXT("Top view orthographic"), Camera.IsOrtho());
	TestTrue(TEXT("Top camera points down"), Camera.GetViewRotation().Vector().Equals(FVector::DownVector, 0.001));
	Camera.FrameTerrain(Preview.GetBounds(), false, true);
	TestTrue(TEXT("Isometric view orthographic"), Camera.IsOrtho());
	Camera.SetViewMode(VMI_Wireframe);
	TestTrue(TEXT("Wireframe rendering selected"), Camera.GetViewMode() == VMI_Wireframe);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainPreviewRenderTest, "ChopIt.Terrain.Preview.Render",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)
bool FChopItTerrainPreviewRenderTest::RunTest(const FString& Parameters)
{
	FChopItTerrainPreview Preview;
	FChopItTerrainSettings Terrain;
	FChopItTerrainMeshSettings Mesh;
	Preview.Request(Terrain, Mesh, 40, 0);
	if (!TestTrue(TEXT("Prepare preview scene"), FinishPreview(Preview, 0.3))) return false;
	if (!TestTrue(TEXT("Preview geometry exists"), !Preview.GetComponents().IsEmpty())) return false;
	UMaterialInterface* Material = Preview.GetComponents()[0]->GetMaterial(0);
	Material->GetMaterial()->ForceRecompileForRendering();
	Material->ForceRecompileForRendering();
	if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
	TSharedRef<FChopItTerrainPreviewViewportClient> CameraOwner = MakeShared<FChopItTerrainPreviewViewportClient>(Preview.GetScene(), nullptr);
	FChopItTerrainPreviewViewportClient& Camera = CameraOwner.Get();
	TSharedRef<SViewport> Widget = SNew(SViewport).RenderDirectlyToWindow(false);
	TSharedRef<FSceneViewport> Viewport = FSceneViewport::Create(CameraOwner, Widget);
	Camera.Viewport = &Viewport.Get();
	// Allocate the offscreen target directly: SetViewportSize requires a native Slate window.
	Viewport->UpdateViewportRHI(false, 960, 720, EWindowMode::Windowed, PF_Unknown);
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("Terrain")), true);
	for (int32 Mode = 0; Mode < 4; ++Mode)
	{
		if (Mode == 1)
		{
			Mesh.Shading = EChopItTerrainShading::Flat;
			Preview.Request(Terrain, Mesh, 40, 1);
			if (!FinishPreview(Preview, 1.3)) return false;
		}
		Camera.FrameTerrain(Preview.GetBounds(), Mode == 2, Mode == 3);
		Camera.SetViewMode(Mode == 3 ? VMI_Wireframe : VMI_Lit);
		for (int32 Frame = 0; Frame < 4; ++Frame)
		{
			++GFrameCounter;
			Preview.GetScene().GetWorld()->SendAllEndOfFrameUpdates();
			Viewport->Draw(false);
			FlushRenderingCommands();
		}
		TArray<FColor> Pixels;
		TestTrue(TEXT("Read rendered preview viewport"), Viewport->ReadPixels(Pixels));
		int32 Visible = 0;
		for (FColor& Pixel : Pixels)
		{
			// Unreal's wireframe view draws dark edges against the non-black viewport background.
			if (Mode == 3 ? FMath::Max3(Pixel.R, Pixel.G, Pixel.B) < 10 : (Pixel.G > Pixel.R * 1.2 && Pixel.G > Pixel.B * 1.2)) ++Visible;
			Pixel.A = 255;
		}
		TestTrue(FString::Printf(TEXT("Preview mode %d shows terrain"), Mode), Visible > (Mode == 3 ? Pixels.Num() / 100 : Pixels.Num() / 20));
		if (Mode == 3) TestTrue(TEXT("Wireframe leaves the background visible"), Visible < Pixels.Num() / 3);
		if (Pixels.Num() == 960 * 720)
		{
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(960, 720, Pixels, PNG);
			const TCHAR* Names[] = { TEXT("Smooth"), TEXT("Flat"), TEXT("Top"), TEXT("Wireframe") };
			const FString Filename = FPaths::ProjectSavedDir() / TEXT("Terrain") / FString::Printf(TEXT("Phase4_%s.png"), Names[Mode]);
			TestTrue(TEXT("Save preview evidence"), FFileHelper::SaveArrayToFile(PNG, *Filename));
		}
	}
	Camera.Viewport = nullptr;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainPreviewMapsTest, "ChopIt.Terrain.Preview.Maps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)
bool FChopItTerrainPreviewMapsTest::RunTest(const FString& Parameters)
{
	FChopItTerrainPreview Preview;
	FChopItTerrainSettings Terrain;
	Terrain.SamplesX = 129; Terrain.SamplesY = 97; Terrain.Length = 15000;
	Terrain.Regions.ClearingCoverage = 0.5f;
	Terrain.Regions.MoistureHeightRange = 250;
	FChopItTerrainMeshSettings MeshSettings;
	MeshSettings.CellsPerChunk = 16;
	Preview.Request(Terrain, MeshSettings, 70, 0);
	if (!TestTrue(TEXT("Prepare mask preview"), FinishPreview(Preview, 0.3))) return false;
	if (!TestNotNull(TEXT("Preview provides spatial maps"), Preview.GetHeightField())) return false;
	const FChopItHeightField* Field = Preview.GetHeightField();
	const float* MapStorage = Field->GetMaps().Clearing.GetData();
	const uint64 Revision = Preview.GetPublishedRevision();
	Preview.SetVisualization(EChopItTerrainPreviewMode::Height);
	UMaterialInterface* Material = Preview.GetComponents()[0]->GetMaterial(0);
	TestTrue(TEXT("Use the engine vertex color material"), Material->GetPathName().Contains(TEXT("VertexColorViewMode_ColorOnly")));
	Material->GetMaterial()->ForceRecompileForRendering();
	Material->ForceRecompileForRendering();
	if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
	TSharedRef<FChopItTerrainPreviewViewportClient> Camera = MakeShared<FChopItTerrainPreviewViewportClient>(Preview.GetScene(), nullptr);
	TSharedRef<SViewport> Widget = SNew(SViewport).RenderDirectlyToWindow(false);
	TSharedRef<FSceneViewport> Viewport = FSceneViewport::Create(Camera, Widget);
	Camera->Viewport = &Viewport.Get();
	Viewport->UpdateViewportRHI(false, 960, 720, EWindowMode::Windowed, PF_Unknown);
	Camera->FrameTerrain(Preview.GetBounds(), true);
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("Terrain")), true);
	for (int32 M = int32(EChopItTerrainPreviewMode::Height); M <= int32(EChopItTerrainPreviewMode::WaterLevel); ++M)
	{
		const auto Mode = EChopItTerrainPreviewMode(M);
		Preview.SetVisualization(Mode);
		TestTrue(TEXT("Display switch preserves heightmap and maps"), Preview.GetHeightField() == Field && MapStorage == Field->GetMaps().Clearing.GetData());
		TestEqual(TEXT("Display switch does not regenerate terrain"), Preview.GetPublishedRevision(), Revision);
		TestFalse(TEXT("Selected map has a legend"), Preview.GetLegend().IsEmpty());
		bool bColorsAligned = true, bGeometryPreserved = true;
		for (UDynamicMeshComponent* Component : Preview.GetComponents())
		{
			const auto& Mesh = Component->GetDynamicMesh()->GetMeshRef();
			const auto* Colors = Mesh.Attributes()->PrimaryColors();
			if (!TestNotNull(TEXT("Map overlay exists on every chunk"), Colors)) return false;
			for (int32 T : Mesh.TriangleIndicesItr())
			{
				const auto Vertices = Mesh.GetTriangle(T);
				const auto Elements = Colors->GetTriangle(T);
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					const FVector3d P = Mesh.GetVertex(Vertices[Corner]);
					const FVector2D XY(P.X, P.Y);
					const FLinearColor Expected = FChopItTerrainPreview::SampleVisualizationColor(*Field, Mode, XY);
					const FVector4f Actual = Colors->GetElement(Elements[Corner]);
					bColorsAligned &= FMath::Abs(Actual.X - Expected.R) < 0.00001f && FMath::Abs(Actual.Y - Expected.G) < 0.00001f && FMath::Abs(Actual.Z - Expected.B) < 0.00001f;
					float H;
					bGeometryPreserved &= Field->SampleLocalHeight(XY, H) && FMath::Abs(P.Z - H) < 0.001;
				}
			}
			TestFalse(TEXT("Mask preview still has no physics"), Component->IsPhysicsStateCreated());
		}
		TestTrue(TEXT("All chunk colors align with source spatial coordinates"), bColorsAligned);
		TestTrue(TEXT("Map views preserve the actual terrain surface"), bGeometryPreserved);
		for (int32 Frame = 0; Frame < 4; ++Frame)
		{
			++GFrameCounter;
			Preview.GetScene().GetWorld()->SendAllEndOfFrameUpdates();
			Viewport->Draw(false);
			FlushRenderingCommands();
		}
		TArray<FColor> Pixels;
		TestTrue(TEXT("Read real rendered mask preview"), Viewport->ReadPixels(Pixels));
		if (Pixels.Num() == 960 * 720)
		{
			const FColor Background = Pixels[0];
			int32 Visible = 0, Chromatic = 0;
			for (FColor& Pixel : Pixels)
			{
				Visible += FMath::Abs(int32(Pixel.R) - Background.R) + FMath::Abs(int32(Pixel.G) - Background.G) + FMath::Abs(int32(Pixel.B) - Background.B) > 60 ? 1 : 0;
				Chromatic += FMath::Max3(Pixel.R, Pixel.G, Pixel.B) - FMath::Min3(Pixel.R, Pixel.G, Pixel.B) > 40 ? 1 : 0;
				Pixel.A = 255;
			}
			TestTrue(FString::Printf(TEXT("Map %d renders visible terrain"), M), Visible > Pixels.Num() / 20);
			if (Mode == EChopItTerrainPreviewMode::Regions || Mode == EChopItTerrainPreviewMode::Height)
				TestTrue(TEXT("Region and height palettes render in color"), Chromatic > Pixels.Num() / 20);
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(960, 720, Pixels, PNG);
			const FString Name = StaticEnum<EChopItTerrainPreviewMode>()->GetNameStringByValue(M);
			TestTrue(TEXT("Save map preview evidence"), FFileHelper::SaveArrayToFile(PNG, *(FPaths::ProjectSavedDir() / TEXT("Terrain") / (TEXT("Phase6_") + Name + TEXT(".png")))));
		}
	}
	Preview.SetVisualization(EChopItTerrainPreviewMode::Surface);
	TestFalse(TEXT("Surface material is restored"), Preview.GetComponents()[0]->GetMaterial(0)->GetPathName().Contains(TEXT("VertexColorViewMode")));
	Camera->Viewport = nullptr;
	return true;
}
#endif
