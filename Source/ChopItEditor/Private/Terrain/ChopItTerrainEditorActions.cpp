#include "Terrain/ChopItTerrainEditorActions.h"
#include "Terrain/ChopItTerrainEditorSettings.h"
#include "Terrain/ChopItTerrainActor.h"
#include "Terrain/ChopItTerrainParameterTools.h"
#include "Terrain/ChopItTerrainDataAsset.h"
#include "Editor.h"
#include "Engine/World.h"
#include "ScopedTransaction.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMeshEditor.h"
#include "DynamicMeshToMeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Factories/DataAssetFactory.h"

namespace { bool ValidateEditorSettings(const UChopItTerrainEditorSettings& Settings, FString& Error); }

UChopItTerrainDataAsset* FChopItTerrainEditorActions::SaveAsDataAsset(
	const UChopItTerrainEditorSettings& Settings, FString& OutError)
{
	OutError.Reset();
	if (!IsInGameThread() || !ValidateEditorSettings(Settings, OutError)) return nullptr;
	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = UChopItTerrainDataAsset::StaticClass();
	UObject* Created = FAssetToolsModule::GetModule().Get().CreateAssetWithDialog(
		TEXT("DA_Terrain"), TEXT("/Game/NewChopIt/Terrain/Generated"), UChopItTerrainDataAsset::StaticClass(), Factory,
		NAME_None, false);
	UChopItTerrainDataAsset* Asset = Cast<UChopItTerrainDataAsset>(Created);
	if (!Asset) { OutError = TEXT("Data Asset creation was cancelled or failed."); return nullptr; }
	Asset->Modify();
	Asset->Terrain.CopyFrom(Settings.Terrain);
	Asset->Mesh = Settings.Mesh;
	Asset->MarkPackageDirty();
	const FString Filename = FPackageName::LongPackageNameToFilename(
		Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	if (!UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, SaveArgs))
	{
		OutError = TEXT("Data Asset was created but could not be saved to disk. Save it from the Content Browser.");
		return nullptr;
	}
	return Asset;
}

UStaticMesh* FChopItTerrainEditorActions::BakeMesh(const AChopItTerrainActor* Target, const FString& BasePackageName, FString& OutError)
{
	OutError.Reset();
	if (!IsInGameThread() || !IsValid(Target) || !Target->GetHeightField().IsValid() || Target->GetTerrainChunks().IsEmpty()
		|| !BasePackageName.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidLongPackageName(BasePackageName))
	{ OutError = TEXT("Bake requires a generated terrain and a valid /Game/ asset path."); return nullptr; }
	UE::Geometry::FDynamicMesh3 Combined;
	Combined.EnableAttributes();
	UE::Geometry::FDynamicMeshEditor MeshEditor(&Combined);
	for (const auto& Chunk : Target->GetTerrainChunks())
	{
		if (!IsValid(Chunk)) { OutError = TEXT("Terrain contains an invalid chunk."); return nullptr; }
		UE::Geometry::FMeshIndexMappings Mappings;
		MeshEditor.AppendMesh(Chunk->GetMesh(), Mappings);
	}
	FMeshDescription Description;
	FStaticMeshAttributes(Description).Register();
	FDynamicMeshToMeshDescription Converter;
	Converter.Convert(&Combined, Description);
	FString PackageName, AssetName;
	FAssetToolsModule::GetModule().Get().CreateUniqueAssetName(BasePackageName, TEXT(""), PackageName, AssetName);
	UPackage* Package = CreatePackage(*PackageName);
	UStaticMesh* Asset = NewObject<UStaticMesh>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	Asset->GetStaticMaterials().Add(FStaticMaterial(Target->GetTerrainChunks()[0]->GetMaterial(0)));
	Asset->AddSourceModel();
	Asset->GetSourceModel(0).BuildSettings.bRecomputeNormals = false;
	Asset->GetSourceModel(0).BuildSettings.bRecomputeTangents = true;
	Asset->GetSourceModel(0).BuildSettings.bGenerateLightmapUVs = false;
	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bAllowCpuAccess = true;
	if (!Asset->BuildFromMeshDescriptions({ &Description }, Params))
	{
		Asset->ClearFlags(RF_Public | RF_Standalone);
		OutError = TEXT("Static mesh build failed."); return nullptr;
	}
	Asset->CreateBodySetup();
	Asset->GetBodySetup()->CollisionTraceFlag = CTF_UseComplexAsSimple;
	Asset->GetBodySetup()->CreatePhysicsMeshes();
	FAssetRegistryModule::AssetCreated(Asset);
	Asset->MarkPackageDirty();
	return Asset;
}

namespace
{
bool ValidateEditorSettings(const UChopItTerrainEditorSettings& Settings, FString& Error)
{
	if (!Settings.Terrain.Validate(Error)) return false;
	if (Settings.Mesh.PolygonDetail < 0 || Settings.Mesh.PolygonDetail > 100 ||
		Settings.Mesh.CellsPerChunk < 8 || Settings.Mesh.CellsPerChunk > 128 ||
		(Settings.Mesh.Shading != EChopItTerrainShading::Smooth && Settings.Mesh.Shading != EChopItTerrainShading::Flat))
	{
		Error = TEXT("Invalid mesh detail, chunk size or shading mode.");
		return false;
	}
	return true;
}

bool CanEditWorld(UWorld* World, FString& Error)
{
	if (!World || World->WorldType != EWorldType::Editor || (GEditor && GEditor->PlayWorld))
	{
		Error = TEXT("Open an editable level and leave Play Mode before generating terrain.");
		return false;
	}
	return true;
}
}

bool FChopItTerrainEditorActions::Regenerate(UWorld* World, AChopItTerrainActor* Target,
	const UChopItTerrainEditorSettings& Settings, FString& OutError)
{
	OutError.Reset();
	if (!CanEditWorld(World, OutError)) return false;
	if (!IsValid(Target) || Target->GetWorld() != World)
	{
		OutError = TEXT("Select an existing terrain actor in this level.");
		return false;
	}
	if (!ValidateEditorSettings(Settings, OutError)) return false;
	const FScopedTransaction Transaction(NSLOCTEXT("ChopItTerrainEditor", "Regenerate", "Regenerate ChopIt Terrain"));
	Target->Modify();
	const FChopItTerrainSettings PreviousTerrain = Target->TerrainSettings;
	const FChopItTerrainMeshSettings PreviousMesh = Target->MeshSettings;
	Target->TerrainSettings = Settings.Terrain;
	Target->MeshSettings = Settings.Mesh;
	if (!Target->RebuildTerrain(OutError))
	{
		Target->TerrainSettings = PreviousTerrain;
		Target->MeshSettings = PreviousMesh;
		return false;
	}
	Target->MarkPackageDirty();
	return true;
}

bool FChopItTerrainEditorActions::Generate(UWorld* World, AChopItTerrainActor*& Target,
	const UChopItTerrainEditorSettings& Settings, FString& OutError)
{
	OutError.Reset();
	if (!CanEditWorld(World, OutError) || !ValidateEditorSettings(Settings, OutError)) return false;
	if (IsValid(Target)) return Regenerate(World, Target, Settings, OutError);
	const FScopedTransaction Transaction(NSLOCTEXT("ChopItTerrainEditor", "Generate", "Generate ChopIt Terrain"));
	AChopItTerrainActor* NewTerrain = World->SpawnActorDeferred<AChopItTerrainActor>(
		AChopItTerrainActor::StaticClass(), FTransform::Identity, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!NewTerrain)
	{
		OutError = TEXT("Could not spawn a terrain actor in the editor level.");
		return false;
	}
	NewTerrain->SetFlags(RF_Transactional);
	NewTerrain->TerrainSettings = Settings.Terrain;
	NewTerrain->MeshSettings = Settings.Mesh;
	NewTerrain->FinishSpawning(FTransform::Identity);
	if (!NewTerrain->GetHeightField().IsValid() || NewTerrain->GetTerrainChunks().IsEmpty())
	{
		OutError = TEXT("Terrain construction failed; check the editor log.");
		NewTerrain->Destroy();
		return false;
	}
	NewTerrain->SetActorLabel(TEXT("ChopIt Forest Terrain"));
	NewTerrain->MarkPackageDirty();
	Target = NewTerrain;
	return true;
}

bool FChopItTerrainEditorActions::ResetDefaults(UChopItTerrainEditorSettings& Settings, FString& OutError)
{
	if (!FChopItTerrainParameterTools::ApplyUnlocked(Settings, FChopItTerrainSettings(), FChopItTerrainMeshSettings(), OutError)) return false;
	Settings.PreviewPolygonDetail = 35;
	Settings.PreviewMap = EChopItTerrainPreviewMode::Surface;
	Settings.SaveConfig();
	return true;
}

bool FChopItTerrainEditorActions::RandomSeed(UChopItTerrainEditorSettings& Settings, FString& OutError)
{
	if (FChopItTerrainParameterTools::IsLocked(Settings, TEXT("Terrain.Seed")))
	{ OutError = TEXT("Unlock Terrain.Seed before changing it."); return false; }
	Settings.Modify();
	Settings.Terrain.Seed = static_cast<int32>(FGuid::NewGuid().A);
	Settings.SaveConfig();
	return true;
}
