#include "Terrain/ChopItTerrainActor.h"
#include "Terrain/ChopItTerrainDataCodec.h"
#include "Terrain/ChopItTerrainGenerator.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainDataTest, "ChopIt.Terrain.Data.PersistenceAndBrush",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainDataTest::RunTest(const FString& Parameters)
{
	FChopItTerrainSettings Settings;
	Settings.SamplesX = 33; Settings.SamplesY = 25;
	FChopItHeightField Original, Restored;
	FString Error;
	if (!TestTrue(TEXT("Generate field"), FChopItTerrainGenerator::Generate(Settings, Original, Error))) return false;
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Encode field"), FChopItTerrainDataCodec::Encode(Original, Bytes, Error))) return false;
	if (!TestTrue(TEXT("Decode field"), FChopItTerrainDataCodec::Decode(Bytes, Restored, Error))) return false;
	TestTrue(TEXT("Exact heights"), Restored.GetHeights() == Original.GetHeights());
	TestTrue(TEXT("Exact clearing mask"), Restored.GetMaps().Clearing == Original.GetMaps().Clearing);
	Bytes[0] ^= 0x7f;
	TestFalse(TEXT("Corruption rejected"), FChopItTerrainDataCodec::Decode(Bytes, Restored, Error));

	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	Context.SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	AChopItTerrainActor* Actor = World->SpawnActor<AChopItTerrainActor>();
	if (!TestNotNull(TEXT("Terrain actor"), Actor)) return false;
	Actor->TerrainSettings = Settings;
	Actor->MeshSettings.PolygonDetail = 0;
	if (!TestTrue(TEXT("Build actor"), Actor->RebuildTerrain(Error))) return false;
	const FVector Center = Actor->GetActorLocation();
	float Before = 0, After = 0, Erased = 0;
	if (!TestTrue(TEXT("Query center"), Actor->SampleWorldMap(EChopItTerrainMap::Clearing, Center, Before))) return false;
	if (!TestTrue(TEXT("Paint center"), Actor->ApplyWorldMaskBrush(EChopItEditableMask::Clearing, EChopItMaskBrush::Paint,
		Center, 3000, 1, Before < 0.5f ? 1.0f : 0.0f, Error))) return false;
	Actor->SampleWorldMap(EChopItTerrainMap::Clearing, Center, After);
	TestTrue(TEXT("Paint changes mask"), FMath::Abs(After - Before) > 0.1f);
	if (!TestTrue(TEXT("Erase center"), Actor->ApplyWorldMaskBrush(EChopItEditableMask::Clearing, EChopItMaskBrush::Erase,
		Center, 3000, 1, 0, Error))) return false;
	Actor->SampleWorldMap(EChopItTerrainMap::Clearing, Center, Erased);
	TestTrue(TEXT("Erase restores procedural mask"), FMath::IsNearlyEqual(Erased, Before, 0.001f));
	const FString Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TerrainDataTest"));
	const FString File = FPaths::Combine(Folder, TEXT("roundtrip.cterrain"));
	IFileManager::Get().MakeDirectory(*Folder, true);
	if (!TestTrue(TEXT("Save data"), Actor->SaveTerrainData(File, Error))) return false;
	if (!TestTrue(TEXT("Export maps"), Actor->ExportTerrainMaps(Folder, Error))) return false;
	AChopItTerrainActor* Loaded = World->SpawnActor<AChopItTerrainActor>();
	if (!TestTrue(TEXT("Load data"), Loaded->LoadTerrainData(File, Error))) return false;
	float LoadedValue = 0;
	Loaded->SampleWorldMap(EChopItTerrainMap::Clearing, Center, LoadedValue);
	TestTrue(TEXT("Loaded edited mask"), FMath::IsNearlyEqual(LoadedValue, Erased, 0.001f));
	TestEqual(TEXT("Loaded stroke history"), Loaded->GetManualStrokeCount(), 2);
	const float TargetValue = Erased < 0.5f ? 1.0f : 0.0f;
	if (!TestTrue(TEXT("Paint loaded terrain"), Loaded->ApplyWorldMaskBrush(EChopItEditableMask::Clearing,
		EChopItMaskBrush::Paint, Center, 3000, 1, TargetValue, Error))) return false;
	Loaded->TerrainSettings.SamplesX = 41;
	Loaded->TerrainSettings.SamplesY = 29;
	if (!TestTrue(TEXT("Regenerate at new resolution"), Loaded->RebuildTerrain(Error))) return false;
	Loaded->SampleWorldMap(EChopItTerrainMap::Clearing, Center, LoadedValue);
	TestTrue(TEXT("Manual paint survives regeneration"), FMath::Abs(LoadedValue - TargetValue) < FMath::Abs(Erased - TargetValue));
	return true;
}
#endif
