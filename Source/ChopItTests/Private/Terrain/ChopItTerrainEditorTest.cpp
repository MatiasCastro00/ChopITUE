#include "Terrain/ChopItTerrainEditorActions.h"
#include "Terrain/ChopItTerrainEditorSettings.h"
#include "Terrain/ChopItTerrainActor.h"
#include "Terrain/ChopItTerrainStatistics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationEditorCommon.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainEditorTest, "ChopIt.Terrain.Editor.Actions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainEditorTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Dockable terrain tab registered"), FGlobalTabmanager::Get()->HasTabSpawner(TEXT("ChopItTerrainGenerator")));
	TSharedPtr<SDockTab> TerrainTab = FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("ChopItTerrainGenerator")));
	TestTrue(TEXT("Terrain settings tab can open"), TerrainTab.IsValid());
	if (TerrainTab.IsValid()) TerrainTab->RequestCloseTab();
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	if (!TestNotNull(TEXT("Editable level"), World)) return false;
	UChopItTerrainEditorSettings* Settings = NewObject<UChopItTerrainEditorSettings>();
	Settings->Terrain.SamplesX = 33;
	Settings->Terrain.SamplesY = 33;
	Settings->Mesh.PolygonDetail = 0;
	AChopItTerrainActor* Target = nullptr;
	FString Error;
	if (!TestTrue(TEXT("Generate from editor settings"), FChopItTerrainEditorActions::Generate(World, Target, *Settings, Error)))
	{
		AddError(Error);
		return false;
	}
	if (!TestNotNull(TEXT("Target actor created"), Target)) return false;
	TestTrue(TEXT("Target has geometry"), !Target->GetTerrainChunks().IsEmpty());
	TestTrue(TEXT("Statistics show generated grid"), FChopItTerrainStatistics::Describe(Target).Contains(TEXT("Grid 33 x 33")));
	int32 ActorCount = 0;
	for (TActorIterator<AChopItTerrainActor> It(World); It; ++It) ++ActorCount;
	TestEqual(TEXT("One terrain actor"), ActorCount, 1);
	AChopItTerrainActor* FirstTarget = Target;
	Settings->Terrain.TargetMeanHeight = 250.0f;
	Settings->Mesh.PolygonDetail = 10;
	TestTrue(TEXT("Generate updates target"), FChopItTerrainEditorActions::Generate(World, Target, *Settings, Error));
	TestTrue(TEXT("Same actor retained"), Target == FirstTarget);
	TestEqual(TEXT("New height parameter applied"), Target->TerrainSettings.TargetMeanHeight, 250.0f);
	TestEqual(TEXT("New detail applied"), Target->MeshSettings.PolygonDetail, 10);
	Settings->Terrain.SamplesX = 0;
	TestFalse(TEXT("Invalid editor settings rejected"), FChopItTerrainEditorActions::Regenerate(World, Target, *Settings, Error));
	TestTrue(TEXT("Invalid settings explain failure"), !Error.IsEmpty());
	TestEqual(TEXT("Actor settings preserved after invalid input"), Target->TerrainSettings.SamplesX, 33);
	ActorCount = 0;
	for (TActorIterator<AChopItTerrainActor> It(World); It; ++It) ++ActorCount;
	TestEqual(TEXT("No actor duplicated"), ActorCount, 1);
	UChopItTerrainEditorSettings* SavedSettings = GetMutableDefault<UChopItTerrainEditorSettings>();
	const FChopItTerrainSettings OriginalTerrain = SavedSettings->Terrain;
	const FChopItTerrainMeshSettings OriginalMesh = SavedSettings->Mesh;
	const TArray<FString> OriginalLocks = SavedSettings->LockedParameters;
	ON_SCOPE_EXIT
	{
		SavedSettings->Terrain = OriginalTerrain;
		SavedSettings->Mesh = OriginalMesh;
		SavedSettings->LockedParameters = OriginalLocks;
		SavedSettings->SaveConfig();
	};
	const int32 StoredSeed = OriginalTerrain.Seed == 873123 ? 873124 : 873123;
	SavedSettings->Terrain.Seed = StoredSeed;
	SavedSettings->Mesh.PolygonDetail = 27;
	SavedSettings->LockedParameters = { TEXT("Terrain.HillStrength") };
	SavedSettings->SaveConfig();
	SavedSettings->Terrain.Seed = 1;
	SavedSettings->Mesh.PolygonDetail = 1;
	SavedSettings->LockedParameters.Reset();
	SavedSettings->ReloadConfig();
	TestEqual(TEXT("Seed restored from editor config"), SavedSettings->Terrain.Seed, StoredSeed);
	TestEqual(TEXT("Mesh detail restored from editor config"), SavedSettings->Mesh.PolygonDetail, 27);
	TestTrue(TEXT("Parameter locks restored from editor config"), SavedSettings->LockedParameters.Contains(TEXT("Terrain.HillStrength")));
	return true;
}
#endif
