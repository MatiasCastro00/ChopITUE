#include "Terrain/ChopItTerrainActor.h"
#include "Terrain/ChopItTerrainDataCodec.h"
#include "Terrain/ChopItTerrainGenerator.h"
#include "Terrain/ChopItTerrainMeshBuilder.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Terrain/ChopItTerrainEditorActions.h"
#include "Components/DynamicMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Level.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "StaticMeshResources.h"
#include "UObject/SavePackage.h"
#include "UObject/Package.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainBenchmark, "ChopIt.Terrain.Final.Benchmark",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainBenchmark::RunTest(const FString& Parameters)
{
	FString CSV = TEXT("samples,generation_ms,encode_ms,decode_ms,mesh_ms,paint_ms,noop_ms,packed_bytes\n");
	FString Error;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	Context.SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	AChopItTerrainActor* Actor = World->SpawnActor<AChopItTerrainActor>();
	for (int32 Resolution : { 257, 513 })
	{
		FChopItTerrainSettings Settings;
		Settings.SamplesX = Settings.SamplesY = Resolution;
		FChopItHeightField Field, Decoded;
		TArray<uint8> Data;
		TArray<FChopItTerrainMeshChunk> Meshes;
		FChopItTerrainMeshSettings Mesh;
		Mesh.bEnableCollision = false;
		double Start = FPlatformTime::Seconds();
		if (!TestTrue(TEXT("Generate"), FChopItTerrainGenerator::Generate(Settings, Field, Error))) return false;
		const double Generation = FPlatformTime::Seconds() - Start;
		Start = FPlatformTime::Seconds();
		if (!TestTrue(TEXT("Encode"), FChopItTerrainDataCodec::Encode(Field, Data, Error))) return false;
		const double Encode = FPlatformTime::Seconds() - Start;
		Start = FPlatformTime::Seconds();
		if (!TestTrue(TEXT("Decode"), FChopItTerrainDataCodec::Decode(Data, Decoded, Error))) return false;
		const double Decode = FPlatformTime::Seconds() - Start;
		Start = FPlatformTime::Seconds();
		if (!TestTrue(TEXT("Mesh"), FChopItTerrainMeshBuilder::Build(Field, Mesh, Meshes, Error))) return false;
		const double Geometry = FPlatformTime::Seconds() - Start;
		Actor->TerrainSettings = Settings; Actor->MeshSettings = Mesh;
		if (!TestTrue(TEXT("Actor"), Actor->RebuildTerrain(Error))) return false;
		Start = FPlatformTime::Seconds();
		if (!TestTrue(TEXT("Paint"), Actor->ApplyWorldMaskBrush(EChopItEditableMask::Clearing,
			EChopItMaskBrush::Paint, FVector::ZeroVector, 500, 1, 1, Error))) return false;
		const double Paint = FPlatformTime::Seconds() - Start;
		Start = FPlatformTime::Seconds();
		if (!TestTrue(TEXT("No-op"), Actor->RebuildTerrain(Error))) return false;
		const double Noop = FPlatformTime::Seconds() - Start;
		FChopItHeightField Repeated;
		TestTrue(TEXT("Repeat parallel generation"), FChopItTerrainGenerator::Generate(Settings, Repeated, Error));
		TestTrue(TEXT("Parallel maps are deterministic"), Repeated.GetMaps().Clearing == Field.GetMaps().Clearing
			&& Repeated.GetMaps().SlopeDegrees == Field.GetMaps().SlopeDegrees && Repeated.GetHeights() == Field.GetHeights());
		CSV += FString::Printf(TEXT("%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d\n"), Resolution,
			Generation * 1000, Encode * 1000, Decode * 1000, Geometry * 1000, Paint * 1000, Noop * 1000, Data.Num());
	}
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("TerrainPhase9");
	IFileManager::Get().MakeDirectory(*Directory, true);
	TestTrue(TEXT("Write timing report"), FFileHelper::SaveStringToFile(CSV, *(Directory / TEXT("Benchmark.csv"))));
	AddInfo(CSV);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainLifecycle, "ChopIt.Terrain.Final.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainLifecycle::RunTest(const FString& Parameters)
{
	UPackage* Package = CreatePackage(TEXT("/Game/TerrainPhase9Fixture"));
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("TerrainPhase9Fixture"), Package);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	World->SetFlags(RF_Public | RF_Standalone);
	AChopItTerrainActor* Actor = World->SpawnActor<AChopItTerrainActor>();
	Actor->SetFlags(RF_Transactional);
	Actor->TerrainSettings.SamplesX = Actor->TerrainSettings.SamplesY = 33;
	Actor->MeshSettings.PolygonDetail = 0;
	FString Error;
	if (!TestTrue(TEXT("Build"), Actor->RebuildTerrain(Error))) return false;
	UDynamicMeshComponent* Chunk = Actor->GetTerrainChunks()[0];
	int32 MeshEvents = 0;
	const FDelegateHandle Handle = Chunk->OnMeshChanged.AddLambda([&MeshEvents]() { ++MeshEvents; });
	Actor->TerrainSettings.Regions.ClearingCoverage = 0.8f;
	TestTrue(TEXT("Analysis-only rebuild"), Actor->RebuildTerrain(Error));
	TestEqual(TEXT("Analysis changes do not republish geometry"), MeshEvents, 0);
	Actor->MeshSettings.bEnableCollision = false;
	TestTrue(TEXT("Collision-only update"), Actor->RebuildTerrain(Error));
	TestEqual(TEXT("Collision changes do not republish geometry"), MeshEvents, 0);
	Chunk->OnMeshChanged.Remove(Handle);
	float Original = 0, Painted = 0;
	Actor->SampleWorldMap(EChopItTerrainMap::Clearing, FVector::ZeroVector, Original);
	{
		const FScopedTransaction Transaction(FText::FromString(TEXT("Phase9 mask undo")));
		TestTrue(TEXT("Paint"), Actor->ApplyWorldMaskBrush(EChopItEditableMask::Clearing, EChopItMaskBrush::Paint,
			FVector::ZeroVector, 2000, 1, 0.75f, Error));
	}
	Actor->SampleWorldMap(EChopItTerrainMap::Clearing, FVector::ZeroVector, Painted);
	TestEqual(TEXT("Paint reaches target"), Painted, 0.75f);
	GEditor->UndoTransaction();
	Actor->SampleWorldMap(EChopItTerrainMap::Clearing, FVector::ZeroVector, Painted);
	TestEqual(TEXT("Undo restores derived mask cache"), Painted, Original);
	GEditor->RedoTransaction();
	Actor->SampleWorldMap(EChopItTerrainMap::Clearing, FVector::ZeroVector, Painted);
	TestEqual(TEXT("Redo restores pending stroke"), Painted, 0.75f);
	TestEqual(TEXT("Redo restores stroke history"), Actor->GetManualStrokeCount(), 1);
	const FVector Local(127, 253, 0);
	Actor->SetActorTransform(FTransform(FRotator(20, 35, -10), FVector(100, 200, 300), FVector(2, 0.5, 3)));
	FVector Normal;
	TestTrue(TEXT("World normal query"), Actor->SampleWorldNormal(Actor->GetActorTransform().TransformPosition(Local), Normal));
	float HX0, HX1, HY0, HY1;
	Actor->GetHeightField().SampleLocalHeight(FVector2D(Local.X - 1, Local.Y), HX0);
	Actor->GetHeightField().SampleLocalHeight(FVector2D(Local.X + 1, Local.Y), HX1);
	Actor->GetHeightField().SampleLocalHeight(FVector2D(Local.X, Local.Y - 1), HY0);
	Actor->GetHeightField().SampleLocalHeight(FVector2D(Local.X, Local.Y + 1), HY1);
	const FVector TX = Actor->GetActorTransform().TransformVector(FVector(2, 0, HX1 - HX0)).GetSafeNormal();
	const FVector TY = Actor->GetActorTransform().TransformVector(FVector(0, 2, HY1 - HY0)).GetSafeNormal();
	TestTrue(TEXT("Normal respects nonuniform scale and rotation"), FMath::Abs(Normal | TX) < 0.0001 && FMath::Abs(Normal | TY) < 0.0001);
	EChopItTerrainRegion Region;
	const FVector Invalid(std::numeric_limits<double>::quiet_NaN(), 0, 0);
	TestFalse(TEXT("NaN region rejected"), Actor->SampleWorldRegion(Invalid, Region));
	TestFalse(TEXT("NaN brush rejected"), Actor->ApplyWorldMaskBrush(EChopItEditableMask::Clearing, EChopItMaskBrush::Paint, Invalid, 100, 1, 1, Error));
	Actor->SetActorScale3D(FVector(0, 1, 1));
	TestFalse(TEXT("Zero scale query rejected"), Actor->SampleWorldNormal(FVector::ZeroVector, Normal));
	Actor->SetActorTransform(FTransform::Identity);
	const FString Folder = FPaths::ProjectSavedDir() / TEXT("TerrainPhase9");
	IFileManager::Get().MakeDirectory(*Folder, true);
	Actor->TerrainSettings.Seed += 7; // Unsaved draft must not relabel the generated snapshot.
	TestTrue(TEXT("Export with draft settings"), Actor->SaveTerrainData(Folder / TEXT("Draft.cterrain"), Error));
	AChopItTerrainActor* Loaded = World->SpawnActor<AChopItTerrainActor>();
	TestTrue(TEXT("Load exported snapshot"), Loaded->LoadTerrainData(Folder / TEXT("Draft.cterrain"), Error));
	TestEqual(TEXT("Export uses generated seed"), Loaded->TerrainSettings.Seed, Actor->GetGeneratedSettings().Seed);
	World->DestroyActor(Loaded);
	Actor->TerrainSettings = Actor->GetGeneratedSettings();
	const auto& BeforeSmooth = Actor->GetHeightField().GetMaps().Clearing;
	const int32 Stride = Actor->GetHeightField().GetSamplesX();
	const float ExpectedCorner = (BeforeSmooth[0] * 4 + BeforeSmooth[1] * 2 + BeforeSmooth[Stride] * 2 + BeforeSmooth[Stride + 1]) / 9;
	const FVector Corner(-Actor->GetHeightField().GetSize().X * 0.5, -Actor->GetHeightField().GetSize().Y * 0.5, 0);
	TestTrue(TEXT("Smooth boundary brush"), Actor->ApplyWorldMaskBrush(EChopItEditableMask::Clearing, EChopItMaskBrush::Smooth, Corner, 1500, 1, 0, Error));
	Actor->SampleWorldMap(EChopItTerrainMap::Clearing, Corner, Painted);
	TestTrue(TEXT("Bounded smooth matches clamped 3x3 neighborhood"), FMath::IsNearlyEqual(Painted, ExpectedCorner, 0.00001f));
	UStaticMesh* Baked = FChopItTerrainEditorActions::BakeMesh(Actor, TEXT("/Game/TerrainPhase9Baked"), Error);
	if (!TestNotNull(TEXT("Bake Static Mesh"), Baked)) { AddError(Error); return false; }
	TestEqual(TEXT("Bake preserves triangles"), int32(Baked->GetRenderData()->LODResources[0].GetNumTriangles()), Chunk->GetMesh()->TriangleCount());
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	TestTrue(TEXT("Save baked asset"), UPackage::SavePackage(Baked->GetOutermost(), Baked, *(Folder / TEXT("Baked.uasset")), Args));
	TestTrue(TEXT("Save level with edited masks"), UPackage::SavePackage(Package, World, *(Folder / TEXT("Roundtrip.umap")), Args));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainReopen, "ChopIt.Terrain.Final.ReopenSavedLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainReopen::RunTest(const FString& Parameters)
{
	const FString Folder = FPaths::ProjectSavedDir() / TEXT("TerrainPhase9");
	UPackage* Package = LoadPackage(nullptr, *(Folder / TEXT("Roundtrip.umap")), LOAD_None);
	if (!TestNotNull(TEXT("Reopen fixture level"), Package)) return false;
	UWorld* World = UWorld::FindWorldInPackage(Package);
	if (!TestNotNull(TEXT("Reopened world"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	AChopItTerrainActor* Actor = nullptr;
	for (AActor* Item : World->PersistentLevel->Actors)
		if (auto* Terrain = Cast<AChopItTerrainActor>(Item)) { Actor = Terrain; break; }
	if (!TestNotNull(TEXT("Persistent terrain actor"), Actor)) return false;
	float Value = 0;
	TestTrue(TEXT("Data restored before construction"), Actor->SampleWorldMap(EChopItTerrainMap::Clearing, FVector::ZeroVector, Value));
	TestEqual(TEXT("Manual mask survives reopening"), Value, 0.75f);
	TestEqual(TEXT("Stroke history persists"), Actor->GetManualStrokeCount(), 2);
	FVector Normal;
	TestTrue(TEXT("Normal data survives reopening"), Actor->SampleWorldNormal(FVector::ZeroVector, Normal));
	TestTrue(TEXT("Chunk geometry persists"), !Actor->GetTerrainChunks().IsEmpty() && Actor->GetTerrainChunks()[0]->GetMesh()->TriangleCount() == 128);
	return true;
}
#endif
