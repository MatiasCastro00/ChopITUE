#include "Terrain/ChopItTerrainPresets.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

bool FChopItTerrainPresets::Build(EChopItTerrainPreset Preset, const FChopItTerrainSettings& Current,
	FChopItTerrainSettings& OutTerrain, FChopItTerrainMeshSettings& OutMesh, FString& OutError)
{
	FChopItTerrainSettings Terrain;
	// Spatial resolution and seed are project choices and remain stable while trying visual styles.
	Terrain.Seed = Current.Seed;
	Terrain.Width = Current.Width;
	Terrain.Length = Current.Length;
	Terrain.SamplesX = Current.SamplesX;
	Terrain.SamplesY = Current.SamplesY;
	switch (Preset)
	{
	case EChopItTerrainPreset::FlatForest:
		Terrain.HeightVariation = 240; Terrain.PlainsAmount = 0.88f; Terrain.PlainsCoverage = 0.80f;
		Terrain.HillStrength = 0.35f; Terrain.ValleyStrength = 0.25f; Terrain.SlopeBias = -0.2f;
		Terrain.Regions.ClearingCoverage = 0.14f; break;
	case EChopItTerrainPreset::GentleForestHills:
		Terrain.HeightVariation = 450; Terrain.PlainsAmount = 0.68f; Terrain.PlainsCoverage = 0.58f;
		Terrain.HillStrength = 0.75f; Terrain.ValleyStrength = 0.55f; Terrain.SlopeBias = -0.1f;
		Terrain.Regions.ClearingCoverage = 0.17f; break;
	case EChopItTerrainPreset::RollingHills:
		Terrain.HeightVariation = 730; Terrain.ReliefScale = 7000; Terrain.PlainsAmount = 0.38f;
		Terrain.PlainsCoverage = 0.32f; Terrain.HillStrength = 1.2f; Terrain.HillShape = 1.4f;
		Terrain.ValleyStrength = 0.75f; Terrain.DomainWarpStrength = 0.18f; break;
	case EChopItTerrainPreset::ForestValleys:
		Terrain.HeightVariation = 680; Terrain.HillStrength = 0.6f; Terrain.ValleyStrength = 1.5f;
		Terrain.ValleyShape = 1.25f; Terrain.DepressionStrength = 0.18f; Terrain.PlainsCoverage = 0.42f;
		Terrain.Regions.MoistureHeightRange = 1400; Terrain.Regions.ClearingCoverage = 0.15f; break;
	case EChopItTerrainPreset::SteepWoodland:
		Terrain.HeightVariation = 1050; Terrain.PlainsAmount = 0.22f; Terrain.PlainsCoverage = 0.20f;
		Terrain.HillStrength = 1.65f; Terrain.ValleyStrength = 0.75f; Terrain.SlopeBias = 0.35f;
		Terrain.Regions.SteepSlopeDegrees = 24; Terrain.Regions.MaxWalkableSlopeDegrees = 42;
		Terrain.Regions.ClearingCoverage = 0.08f; break;
	case EChopItTerrainPreset::WetLowlands:
		Terrain.HeightVariation = 320; Terrain.PlainsAmount = 0.76f; Terrain.PlainsCoverage = 0.70f;
		Terrain.HillStrength = 0.4f; Terrain.ValleyStrength = 0.8f; Terrain.DepressionStrength = 0.28f;
		Terrain.bAutoWaterLevel = true; Terrain.TargetSubmergedFraction = 0.25f;
		Terrain.Regions.MoistureHeightRange = 1800; Terrain.Regions.ClearingCoverage = 0.09f; break;
	case EChopItTerrainPreset::ForestClearings:
		Terrain.HeightVariation = 470; Terrain.PlainsAmount = 0.76f; Terrain.PlainsCoverage = 0.70f;
		Terrain.HillStrength = 0.72f; Terrain.ValleyStrength = 0.42f;
		Terrain.Regions.RegionScale = 3600; Terrain.Regions.ClearingCoverage = 0.58f;
		Terrain.Regions.TransitionWidth = 0.18f; break;
	default:
		OutError = TEXT("Unknown terrain preset."); return false;
	}
	if (!Terrain.Validate(OutError)) return false;
	OutTerrain = MoveTemp(Terrain);
	OutMesh = FChopItTerrainMeshSettings();
	return true;
}

bool FChopItTerrainPresets::Save(const FString& Path, const FChopItTerrainSettings& Terrain,
	const FChopItTerrainMeshSettings& Mesh, FString& OutError)
{
	if (Path.IsEmpty() || !Terrain.Validate(OutError))
	{ if (OutError.IsEmpty()) OutError = TEXT("Preset path is empty."); return false; }
	FChopItTerrainPresetFile File;
	File.Terrain = Terrain; File.Mesh = Mesh;
	FString Json;
	if (!FJsonObjectConverter::UStructToJsonObjectString(File, Json))
	{ OutError = TEXT("Unable to serialize preset."); return false; }
	if (!FFileHelper::SaveStringToFile(Json, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{ OutError = TEXT("Unable to write preset file."); return false; }
	return true;
}

bool FChopItTerrainPresets::Load(const FString& Path, FChopItTerrainSettings& OutTerrain,
	FChopItTerrainMeshSettings& OutMesh, FString& OutError)
{
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *Path)) { OutError = TEXT("Unable to read preset file."); return false; }
	FChopItTerrainPresetFile File;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &File, 0, 0) || File.Version != 1)
	{ OutError = TEXT("Unsupported or invalid preset file."); return false; }
	if (!File.Terrain.Validate(OutError)) return false;
	if (File.Mesh.PolygonDetail < 0 || File.Mesh.PolygonDetail > 100 || File.Mesh.CellsPerChunk < 8 || File.Mesh.CellsPerChunk > 128
		|| (File.Mesh.Shading != EChopItTerrainShading::Flat && File.Mesh.Shading != EChopItTerrainShading::Smooth))
	{ OutError = TEXT("Invalid mesh controls in preset."); return false; }
	OutTerrain = MoveTemp(File.Terrain);
	OutMesh = MoveTemp(File.Mesh);
	return true;
}
