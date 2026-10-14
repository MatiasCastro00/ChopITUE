#include "Terrain/ChopItTerrainEditorSettings.h"
#include "Terrain/ChopItTerrainParameterTools.h"
#include "Terrain/ChopItTerrainEditorActions.h"
#include "Terrain/ChopItTerrainPresets.h"
#include "Terrain/ChopItTerrainGenerator.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainPhase8Test, "ChopIt.Terrain.Editor.PresetsAndParameters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainPhase8Test::RunTest(const FString& Parameters)
{
	UChopItTerrainEditorSettings* Settings = NewObject<UChopItTerrainEditorSettings>();
	Settings->Terrain = FChopItTerrainSettings(); Settings->Mesh = FChopItTerrainMeshSettings();
	Settings->LockedParameters.Reset();
	Settings->Terrain.Seed = 42877;
	Settings->Terrain.SamplesX = 65; Settings->Terrain.SamplesY = 49;
	TSet<FString> Signatures;
	double FlatMeanSlope = 0, SteepMeanSlope = 0;
	FString Error;
	for (int32 Index = 0; Index < 7; ++Index)
	{
		FChopItTerrainSettings Terrain; FChopItTerrainMeshSettings Mesh;
		if (!TestTrue(TEXT("Build curated preset"), FChopItTerrainPresets::Build(
			EChopItTerrainPreset(Index), Settings->Terrain, Terrain, Mesh, Error))) return false;
		TestEqual(TEXT("Preset keeps seed"), Terrain.Seed, 42877);
		TestEqual(TEXT("Preset keeps X resolution"), Terrain.SamplesX, 65);
		TestTrue(TEXT("Preset validates"), Terrain.Validate(Error));
		FChopItHeightField Field;
		if (!TestTrue(TEXT("Preset generates"), FChopItTerrainGenerator::Generate(Terrain, Field, Error))) return false;
		if (Index == int32(EChopItTerrainPreset::FlatForest) || Index == int32(EChopItTerrainPreset::SteepWoodland))
		{
			double Sum = 0;
			for (float Slope : Field.GetMaps().SlopeDegrees) Sum += Slope;
			if (Index == int32(EChopItTerrainPreset::FlatForest)) FlatMeanSlope = Sum / Field.GetMaps().SlopeDegrees.Num();
			else SteepMeanSlope = Sum / Field.GetMaps().SlopeDegrees.Num();
		}
		Signatures.Add(FString::Printf(TEXT("%.1f|%.2f|%.2f|%.2f|%.2f"), Terrain.HeightVariation,
			Terrain.HillStrength, Terrain.ValleyStrength, Terrain.PlainsCoverage, Terrain.Regions.ClearingCoverage));
		if (Index == int32(EChopItTerrainPreset::WetLowlands))
			TestTrue(TEXT("Wet preset creates submerged area"), Field.GetSubmergedFraction() > 0.1);
	}
	TestEqual(TEXT("Seven distinct presets"), Signatures.Num(), 7);
	TestTrue(TEXT("Steep woodland is steeper than flat forest"), SteepMeanSlope > FlatMeanSlope * 1.5);
	TestTrue(TEXT("Paths include nested regions"), FChopItTerrainParameterTools::GetParameterPaths().Contains(TEXT("Terrain.Regions.ClearingCoverage")));
	TestTrue(TEXT("Category list includes Hills"), FChopItTerrainParameterTools::GetCategories().Contains(TEXT("Hills")));
	if (!TestTrue(TEXT("Lock HillStrength"), FChopItTerrainParameterTools::ToggleLock(*Settings, TEXT("Terrain.HillStrength"), Error))) return false;
	const float LockedHill = Settings->Terrain.HillStrength;
	FChopItTerrainSettings Flat; FChopItTerrainMeshSettings FlatMesh;
	FChopItTerrainPresets::Build(EChopItTerrainPreset::FlatForest, Settings->Terrain, Flat, FlatMesh, Error);
	if (!TestTrue(TEXT("Apply preset with lock"), FChopItTerrainParameterTools::ApplyUnlocked(*Settings, Flat, FlatMesh, Error))) return false;
	TestEqual(TEXT("Locked parameter kept"), Settings->Terrain.HillStrength, LockedHill);
	TestEqual(TEXT("Unlocked parameter changed"), Settings->Terrain.HeightVariation, 240.0f);
	Settings->Terrain.HillShape = 2.75f;
	if (!TestTrue(TEXT("Randomize Hills"), FChopItTerrainParameterTools::Randomize(*Settings, TEXT("Hills"), Error))) return false;
	TestEqual(TEXT("Category randomization keeps lock"), Settings->Terrain.HillStrength, LockedHill);
	if (!TestTrue(TEXT("Randomize all settings"), FChopItTerrainParameterTools::Randomize(*Settings, TEXT("All"), Error))) return false;
	TestEqual(TEXT("Global randomization keeps lock"), Settings->Terrain.HillStrength, LockedHill);
	TestTrue(TEXT("Randomized settings validate"), Settings->Terrain.Validate(Error));
	TestFalse(TEXT("Locked parameter cannot reset"), FChopItTerrainParameterTools::ResetParameter(*Settings, TEXT("Terrain.HillStrength"), Error));
	if (!TestTrue(TEXT("Reset one parameter"), FChopItTerrainParameterTools::ResetParameter(*Settings, TEXT("Terrain.HillShape"), Error))) return false;
	TestEqual(TEXT("Single default restored"), Settings->Terrain.HillShape, FChopItTerrainSettings().HillShape);
	const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TerrainPresetTest"));
	IFileManager::Get().MakeDirectory(*Directory, true);
	const FString File = FPaths::Combine(Directory, TEXT("forest.cpreset"));
	if (!TestTrue(TEXT("Save preset"), FChopItTerrainPresets::Save(File, Settings->Terrain, Settings->Mesh, Error))) return false;
	FChopItTerrainSettings Loaded; FChopItTerrainMeshSettings LoadedMesh;
	if (!TestTrue(TEXT("Load preset"), FChopItTerrainPresets::Load(File, Loaded, LoadedMesh, Error))) return false;
	TestTrue(TEXT("Preset terrain round-trip"), FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&Settings->Terrain, &Loaded, 0));
	TestTrue(TEXT("Preset mesh round-trip"), FChopItTerrainMeshSettings::StaticStruct()->CompareScriptStruct(&Settings->Mesh, &LoadedMesh, 0));
	TestTrue(TEXT("Unlock parameter"), FChopItTerrainParameterTools::ToggleLock(*Settings, TEXT("Terrain.HillStrength"), Error));
	return true;
}
#endif
