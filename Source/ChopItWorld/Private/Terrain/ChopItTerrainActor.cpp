#include "Terrain/ChopItTerrainActor.h"
#include "Terrain/ChopItTerrainGenerator.h"
#include "Terrain/ChopItTerrainMeshBuilder.h"
#include "Terrain/ChopItTerrainDataCodec.h"
#include "Terrain/ChopItTerrainAnalysis.h"
#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "JsonObjectConverter.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "UObject/ObjectSaveContext.h"

namespace
{
bool ValidQuery(const AActor& Actor, const FVector& Position)
{
	const FVector Scale = Actor.GetActorScale3D();
	return !Position.ContainsNaN() && !Actor.GetActorTransform().ContainsNaN()
		&& FMath::Abs(Scale.X) > UE_SMALL_NUMBER && FMath::Abs(Scale.Y) > UE_SMALL_NUMBER && FMath::Abs(Scale.Z) > UE_SMALL_NUMBER;
}
}

AChopItTerrainActor::AChopItTerrainActor()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TerrainRoot"));
	RootComponent->SetMobility(EComponentMobility::Static);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Ground(TEXT("/Game/NewChopIt/Art/Blockout/Materials/MI_Ground.MI_Ground"));
	TerrainMaterial = Ground.Object;
}

void AChopItTerrainActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const bool bSameSettings = FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&TerrainSettings, &GeneratedSettings, 0)
		&& FChopItTerrainMeshSettings::StaticStruct()->CompareScriptStruct(&MeshSettings, &GeneratedMeshSettings, 0);
	if (HeightField.IsValid() && bSameSettings && GeneratedMaterial.Get() == TerrainMaterial
		&& !TerrainChunks.IsEmpty() && !TerrainChunks.ContainsByPredicate([](const auto& Chunk) { return !IsValid(Chunk); })) return;
	RegenerateTerrain();
}

#if WITH_EDITOR
void AChopItTerrainActor::PostEditUndo()
{
	Super::PostEditUndo();
	// UObject transactions restore the serialized snapshot, not the derived C++ caches.
	HeightField = FChopItHeightField();
	RegenerateTerrain();
}
#endif

void AChopItTerrainActor::PostLoad()
{
	Super::PostLoad();
	FString Error;
	if (RestoreSnapshot(HeightField, Error))
	{
		GeneratedSettings = SavedTerrainSettings;
		GeneratedMeshSettings = MeshSettings;
		GeneratedMaterial = TerrainMaterial;
	}
}

bool AChopItTerrainActor::RestoreSnapshot(FChopItHeightField& OutField, FString& OutError) const
{
	if (!FChopItTerrainDataCodec::Decode(SavedTerrainData, OutField, OutError)) return false;
	if (SnapshotStrokeCount < 0) return true;
	if (SnapshotStrokeCount > ManualStrokes.Num()) { OutError = TEXT("Invalid snapshot stroke count."); return false; }
	FChopItHeightField Base;
	for (int32 I = SnapshotStrokeCount; I < ManualStrokes.Num(); ++I)
	{
		if (ManualStrokes[I].Brush == EChopItMaskBrush::Erase && !Base.IsValid()
			&& !FChopItTerrainGenerator::Generate(SavedTerrainSettings, Base, OutError)) return false;
		if (!ApplyStroke(OutField, Base, ManualStrokes[I])) return false;
	}
	return true;
}

void AChopItTerrainActor::PreSave(FObjectPreSaveContext SaveContext)
{
	if (HeightField.IsValid() && SnapshotStrokeCount != ManualStrokes.Num())
	{
		FString Error;
		if (FChopItTerrainDataCodec::Encode(HeightField, SavedTerrainData, Error))
		{
			SavedTerrainSettings = GeneratedSettings;
			SnapshotStrokeCount = ManualStrokes.Num();
		}
		else UE_LOG(LogChopIt, Error, TEXT("Terrain save snapshot failed: %s"), *Error);
	}
	Super::PreSave(SaveContext);
}

void AChopItTerrainActor::RegenerateTerrain()
{
	FString Error;
	if (!RebuildTerrain(Error)) UE_LOG(LogChopIt, Warning, TEXT("Terrain generation failed: %s"), *Error);
}

bool AChopItTerrainActor::RebuildTerrain(FString& OutError)
{
	OutError.Reset();
	if (!IsInGameThread())
	{
		OutError = TEXT("Terrain components must be updated on the game thread.");
		return false;
	}
	if (!TerrainSettings.Validate(OutError)) return false;
	const bool bNewField = !HeightField.IsValid() || !FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&TerrainSettings, &GeneratedSettings, 0);
	FChopItHeightField CandidateField;
	TArray<uint8> CandidateData;
	if (bNewField)
	{
		const bool bSavedSettingsMatch = FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&TerrainSettings, &SavedTerrainSettings, 0);
		if (!bSavedSettingsMatch || SavedTerrainData.IsEmpty()
			|| !RestoreSnapshot(CandidateField, OutError)
			|| CandidateField.GetSamplesX() != TerrainSettings.SamplesX || CandidateField.GetSamplesY() != TerrainSettings.SamplesY)
		{
			FChopItTerrainSettings HeightSettings = TerrainSettings;
			HeightSettings.Regions = GeneratedSettings.Regions;
			const bool bOnlyRegions = HeightField.IsValid()
				&& FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&HeightSettings, &GeneratedSettings, 0);
			if (bOnlyRegions)
			{
				CandidateField = HeightField;
				if (!FChopItTerrainAnalysis::Build(CandidateField.Heights,
					FIntPoint(CandidateField.SamplesX, CandidateField.SamplesY), CandidateField.Size,
					CandidateField.WaterLevel, TerrainSettings.Seed, TerrainSettings.Regions, CandidateField.Maps, OutError)) return false;
			}
			else if (!FChopItTerrainGenerator::Generate(TerrainSettings, CandidateField, OutError)) return false;
			FChopItHeightField BaseField;
			if (ManualStrokes.ContainsByPredicate([](const auto& Stroke) { return Stroke.Brush == EChopItMaskBrush::Erase; }))
				BaseField = CandidateField;
			for (const FChopItMaskStroke& Stroke : ManualStrokes) ApplyStroke(CandidateField, BaseField, Stroke);
		}
		if (!FChopItTerrainDataCodec::Encode(CandidateField, CandidateData, OutError)) return false;
	}
	const bool bSameSurface = HeightField.IsValid() && (!bNewField
		|| (CandidateField.Size == HeightField.Size && CandidateField.SamplesX == HeightField.SamplesX
			&& CandidateField.SamplesY == HeightField.SamplesY && CandidateField.Heights == HeightField.Heights));
	const bool bReplaceGeometry = !bSameSurface || TerrainChunks.IsEmpty()
		|| TerrainChunks.ContainsByPredicate([](const auto& Chunk) { return !IsValid(Chunk); })
		|| MeshSettings.PolygonDetail != GeneratedMeshSettings.PolygonDetail
		|| MeshSettings.CellsPerChunk != GeneratedMeshSettings.CellsPerChunk || MeshSettings.Shading != GeneratedMeshSettings.Shading;
	TArray<FChopItTerrainMeshChunk> Meshes;
	if (bReplaceGeometry && !FChopItTerrainMeshBuilder::Build(bNewField ? CandidateField : HeightField, MeshSettings, Meshes, OutError)) return false;
	if (!bNewField && !bReplaceGeometry && MeshSettings.bEnableCollision == GeneratedMeshSettings.bEnableCollision
		&& GeneratedMaterial.Get() == TerrainMaterial) return true;

	Modify();
	PublishMeshes(Meshes, bReplaceGeometry);
	if (bNewField)
	{
		HeightField = MoveTemp(CandidateField);
		SavedTerrainData = MoveTemp(CandidateData);
		SavedTerrainSettings = TerrainSettings;
		SnapshotStrokeCount = ManualStrokes.Num();
	}
	GeneratedSettings = TerrainSettings;
	GeneratedMeshSettings = MeshSettings;
	GeneratedMaterial = TerrainMaterial;
	return true;
}

void AChopItTerrainActor::PublishMeshes(TArray<FChopItTerrainMeshChunk>& Meshes, bool bReplaceGeometry)
{
	while (bReplaceGeometry && TerrainChunks.Num() > Meshes.Num())
	{
		UDynamicMeshComponent* Extra = TerrainChunks.Pop();
		if (IsValid(Extra)) { RemoveInstanceComponent(Extra); Extra->DestroyComponent(); }
	}
	if (bReplaceGeometry) TerrainChunks.SetNum(Meshes.Num());
	for (int32 Index = 0; Index < TerrainChunks.Num(); ++Index)
	{
		UDynamicMeshComponent* Component = TerrainChunks[Index];
		if (!IsValid(Component))
		{
			Component = NewObject<UDynamicMeshComponent>(this, NAME_None, RF_Transactional);
			Component->SetupAttachment(RootComponent);
			Component->SetMobility(EComponentMobility::Static);
			AddInstanceComponent(Component);
			TerrainChunks[Index] = Component;
			Component->RegisterComponent();
		}
		Component->Modify();
		if (!bReplaceGeometry && MeshSettings.bEnableCollision == GeneratedMeshSettings.bEnableCollision)
		{
			Component->SetMaterial(0, TerrainMaterial ? TerrainMaterial.Get() : UMaterial::GetDefaultMaterial(MD_Surface));
			continue;
		}
		Component->SetDeferredCollisionUpdatesEnabled(true, false);
		Component->bUseAsyncCooking = false; // Publish complete collision before reporting a successful rebuild.
		Component->SetCollisionProfileName(TEXT("BlockAll"));
		Component->SetCollisionObjectType(ECC_WorldStatic);
		Component->SetCollisionEnabled(MeshSettings.bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Component->SetComplexAsSimpleCollisionEnabled(MeshSettings.bEnableCollision, false);
		Component->SetCanEverAffectNavigation(MeshSettings.bEnableCollision);
		Component->ComponentTags.AddUnique(ChopItCollisionChannels::CameraSolidTag);
		Component->SetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid, ECR_Block);
		Component->SetMaterial(0, TerrainMaterial ? TerrainMaterial.Get() : UMaterial::GetDefaultMaterial(MD_Surface));
		Component->SetTangentsType(EDynamicMeshComponentTangentsMode::AutoCalculated);
		if (bReplaceGeometry) Component->SetMesh(MoveTemp(*Meshes[Index].Mesh));
		if (MeshSettings.bEnableCollision) Component->UpdateCollision(false);
	}
}

bool AChopItTerrainActor::SampleWorldMap(EChopItTerrainMap Map, FVector WorldPosition, float& OutValue) const
{
	OutValue = 0;
	if (!ValidQuery(*this, WorldPosition)) return false;
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldPosition);
	return HeightField.SampleLocalMap(Map, FVector2D(Local.X, Local.Y), OutValue);
}

bool AChopItTerrainActor::SampleWorldHeight(FVector WorldPosition, float& OutHeight) const
{
	OutHeight = 0;
	float LocalHeight = 0;
	if (!SampleWorldMap(EChopItTerrainMap::Height, WorldPosition, LocalHeight)) return false;
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldPosition);
	OutHeight = GetActorTransform().TransformPosition(FVector(Local.X, Local.Y, LocalHeight)).Z;
	return true;
}

bool AChopItTerrainActor::SampleWorldNormal(FVector WorldPosition, FVector& OutNormal) const
{
	OutNormal = FVector::ZeroVector;
	if (!ValidQuery(*this, WorldPosition)) return false;
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldPosition);
	int32 I; FVector2D Alpha;
	if (!HeightField.GetSampleCoordinates(FVector2D(Local.X, Local.Y), I, Alpha)) return false;
	const auto& H = HeightField.Heights;
	const int32 S = HeightField.SamplesX;
	const double DX = FMath::Lerp(double(H[I + 1]) - H[I], double(H[I + S + 1]) - H[I + S], Alpha.Y)
		* (S - 1) / HeightField.Size.X;
	const double DY = FMath::Lerp(double(H[I + S]) - H[I], double(H[I + S + 1]) - H[I + 1], Alpha.X)
		* (HeightField.SamplesY - 1) / HeightField.Size.Y;
	OutNormal = GetActorQuat().RotateVector(FVector(-DX, -DY, 1) / GetActorScale3D()).GetSafeNormal();
	return !OutNormal.IsNearlyZero();
}

bool AChopItTerrainActor::SampleWorldRegion(FVector WorldPosition, EChopItTerrainRegion& OutRegion) const
{
	OutRegion = EChopItTerrainRegion::Transitional;
	if (!ValidQuery(*this, WorldPosition) || !HeightField.IsValid() || !HeightField.GetMaps().IsValid(HeightField.GetHeights().Num())) return false;
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldPosition);
	if (Local.ContainsNaN()) return false;
	const FVector2D Size = HeightField.GetSize();
	if (FMath::Abs(Local.X) > Size.X * 0.5 || FMath::Abs(Local.Y) > Size.Y * 0.5) return false;
	const int32 X = FMath::Clamp(FMath::RoundToInt((Local.X / Size.X + 0.5) * (HeightField.GetSamplesX() - 1)), 0, HeightField.GetSamplesX() - 1);
	const int32 Y = FMath::Clamp(FMath::RoundToInt((Local.Y / Size.Y + 0.5) * (HeightField.GetSamplesY() - 1)), 0, HeightField.GetSamplesY() - 1);
	OutRegion = HeightField.GetMaps().Regions[X + Y * HeightField.GetSamplesX()];
	return true;
}

bool AChopItTerrainActor::SampleWorldRegionWeights(FVector WorldPosition, TArray<float>& OutWeights) const
{
	OutWeights.Reset();
	if (!ValidQuery(*this, WorldPosition)) return false;
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldPosition);
	FChopItTerrainRegionBlend Blend;
	if (!HeightField.SampleLocalRegionBlend(FVector2D(Local.X, Local.Y), Blend)) return false;
	OutWeights.Append(Blend.Weights, FChopItTerrainRegionBlend::Count);
	return true;
}

bool AChopItTerrainActor::ApplyStroke(FChopItHeightField& Field, const FChopItHeightField& BaseField, const FChopItMaskStroke& Stroke) const
{
	TArray<float>* Layer = nullptr;
	const TArray<float>* BaseLayer = nullptr;
	switch (Stroke.Mask)
	{
	case EChopItEditableMask::Walkability: Layer = &Field.Maps.Walkability; BaseLayer = &BaseField.Maps.Walkability; break;
	case EChopItEditableMask::Clearing: Layer = &Field.Maps.Clearing; BaseLayer = &BaseField.Maps.Clearing; break;
	case EChopItEditableMask::Moisture: Layer = &Field.Maps.MoisturePotential; BaseLayer = &BaseField.Maps.MoisturePotential; break;
	case EChopItEditableMask::VegetationDensity: Layer = &Field.Maps.VegetationDensityPotential; BaseLayer = &BaseField.Maps.VegetationDensityPotential; break;
	}
	if (!Layer || Layer->Num() != Field.Heights.Num()
		|| (Stroke.Brush == EChopItMaskBrush::Erase && (!BaseLayer || BaseLayer->Num() != Layer->Num()))) return false;
	const double Radius = FMath::Max(0.0001f, Stroke.RadiusFraction);
	const double AspectX = Field.Size.X / FMath::Min(Field.Size.X, Field.Size.Y);
	const double AspectY = Field.Size.Y / FMath::Min(Field.Size.X, Field.Size.Y);
	// Clamp in UV space before converting to integers, including very large brush radii.
	const int32 MinX = FMath::FloorToInt(FMath::Clamp(Stroke.CenterUV.X - Radius / AspectX, 0.0, 1.0) * (Field.SamplesX - 1));
	const int32 MaxX = FMath::CeilToInt(FMath::Clamp(Stroke.CenterUV.X + Radius / AspectX, 0.0, 1.0) * (Field.SamplesX - 1));
	const int32 MinY = FMath::FloorToInt(FMath::Clamp(Stroke.CenterUV.Y - Radius / AspectY, 0.0, 1.0) * (Field.SamplesY - 1));
	const int32 MaxY = FMath::CeilToInt(FMath::Clamp(Stroke.CenterUV.Y + Radius / AspectY, 0.0, 1.0) * (Field.SamplesY - 1));
	// Smoothing only needs the affected rectangle and a one-sample halo.
	const int32 CopyX = FMath::Max(0, MinX - 1), CopyY = FMath::Max(0, MinY - 1);
	const int32 CopyWidth = FMath::Min(Field.SamplesX - 1, MaxX + 1) - CopyX + 1;
	TArray<float> Before;
	if (Stroke.Brush == EChopItMaskBrush::Smooth)
	{
		const int32 CopyHeight = FMath::Min(Field.SamplesY - 1, MaxY + 1) - CopyY + 1;
		Before.Reserve(CopyWidth * CopyHeight);
		for (int32 Row = CopyY; Row < CopyY + CopyHeight; ++Row)
			Before.Append(Layer->GetData() + CopyX + Row * Field.SamplesX, CopyWidth);
	}
	for (int32 Y = MinY; Y <= MaxY; ++Y)
	for (int32 X = MinX; X <= MaxX; ++X)
	{
		const double DX = (double(X) / (Field.SamplesX - 1) - Stroke.CenterUV.X) * AspectX;
		const double DY = (double(Y) / (Field.SamplesY - 1) - Stroke.CenterUV.Y) * AspectY;
		const double D = FMath::Sqrt(DX * DX + DY * DY) / Radius;
		if (D >= 1) continue;
		const int32 I = X + Y * Field.SamplesX;
		const float Weight = FMath::Clamp(Stroke.Strength, 0.0f, 1.0f) * float(1.0 - FMath::SmoothStep(0.0, 1.0, D));
		float Target = FMath::Clamp(Stroke.TargetValue, 0.0f, 1.0f);
		if (Stroke.Brush == EChopItMaskBrush::Erase) Target = (*BaseLayer)[I];
		if (Stroke.Brush == EChopItMaskBrush::Smooth)
		{
			float Sum = 0; int32 N = 0;
			for (int32 DY2 = -1; DY2 <= 1; ++DY2)
			for (int32 DX2 = -1; DX2 <= 1; ++DX2)
			{
				const int32 SX = FMath::Clamp(X + DX2, 0, Field.SamplesX - 1);
				const int32 SY = FMath::Clamp(Y + DY2, 0, Field.SamplesY - 1);
				Sum += Before[(SX - CopyX) + (SY - CopyY) * CopyWidth]; ++N;
			}
			Target = Sum / N;
		}
		(*Layer)[I] = FMath::Clamp(FMath::Lerp((*Layer)[I], Target, Weight), 0.0f, 1.0f);
	}
	return true;
}

bool AChopItTerrainActor::ApplyWorldMaskBrush(EChopItEditableMask Mask, EChopItMaskBrush Brush, FVector WorldPosition,
	float RadiusCm, float Strength, float TargetValue, FString& OutError)
{
	OutError.Reset();
	if (!IsInGameThread() || !HeightField.IsValid() || !ValidQuery(*this, WorldPosition) || !FMath::IsFinite(RadiusCm) || RadiusCm <= 0
		|| !FMath::IsFinite(Strength) || !FMath::IsFinite(TargetValue) || uint8(Mask) > 3 || uint8(Brush) > 2)
	{ OutError = TEXT("Invalid brush or missing terrain data."); return false; }
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldPosition);
	const FVector2D Size = HeightField.GetSize();
	const FVector Scale = GetActorScale3D();
	const double ShortWorldSide = FMath::Min(Size.X * FMath::Abs(Scale.X), Size.Y * FMath::Abs(Scale.Y));
	if (Local.ContainsNaN() || ShortWorldSide <= UE_SMALL_NUMBER || !FMath::IsFinite(ShortWorldSide))
	{ OutError = TEXT("Terrain horizontal scale is invalid."); return false; }
	FChopItMaskStroke Stroke;
	Stroke.Mask = Mask; Stroke.Brush = Brush;
	Stroke.CenterUV = FVector2D(Local.X / Size.X + 0.5, Local.Y / Size.Y + 0.5);
	Stroke.RadiusFraction = RadiusCm / ShortWorldSide;
	if (!FMath::IsFinite(Stroke.RadiusFraction)) { OutError = TEXT("Brush radius is invalid."); return false; }
	Stroke.Strength = FMath::Clamp(Strength, 0.0f, 1.0f);
	Stroke.TargetValue = FMath::Clamp(TargetValue, 0.0f, 1.0f);
	if (Stroke.CenterUV.X < 0 || Stroke.CenterUV.X > 1 || Stroke.CenterUV.Y < 0 || Stroke.CenterUV.Y > 1 || ManualStrokes.Num() >= 4096)
	{ OutError = TEXT("Brush center is outside the terrain or the stroke limit was reached."); return false; }
	FChopItHeightField BaseField;
	if (Brush == EChopItMaskBrush::Erase && !FChopItTerrainGenerator::Generate(GeneratedSettings, BaseField, OutError)) return false;
	if (!HeightField.Maps.IsValid(HeightField.Heights.Num())) { OutError = TEXT("Missing terrain masks."); return false; }
	Modify();
	if (SnapshotStrokeCount < 0) SnapshotStrokeCount = ManualStrokes.Num();
	if (!ApplyStroke(HeightField, BaseField, Stroke)) { OutError = TEXT("Mask edit failed."); return false; }
	SavedTerrainSettings = GeneratedSettings;
	ManualStrokes.Add(Stroke);
	MarkPackageDirty();
	return true;
}

bool AChopItTerrainActor::SaveTerrainData(const FString& FilePath, FString& OutError) const
{
	OutError.Reset();
	if (!HeightField.IsValid() || FilePath.IsEmpty()) { OutError = TEXT("Terrain or file path is invalid."); return false; }
	TArray<uint8> Snapshot;
	if (!FChopItTerrainDataCodec::Encode(HeightField, Snapshot, OutError)) return false;
	FChopItTerrainSaveFile Save;
	Save.TerrainSettings = GeneratedSettings;
	Save.MeshSettings = GeneratedMeshSettings;
	Save.ManualStrokes = ManualStrokes;
	Save.DataBase64 = FBase64::Encode(Snapshot);
	FString Json;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Save, Json))
	{ OutError = TEXT("Unable to serialize terrain settings."); return false; }
	if (!FFileHelper::SaveStringToFile(Json, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{ OutError = TEXT("Unable to write terrain file."); return false; }
	return true;
}

bool AChopItTerrainActor::LoadTerrainData(const FString& FilePath, FString& OutError)
{
	OutError.Reset();
	if (!IsInGameThread()) { OutError = TEXT("Terrain loading must run on the game thread."); return false; }
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *FilePath)) { OutError = TEXT("Unable to read terrain file."); return false; }
	FChopItTerrainSaveFile Save;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Save, 0, 0) || Save.Version != 1
		|| !Save.TerrainSettings.Validate(OutError))
	{ if (OutError.IsEmpty()) OutError = TEXT("Unsupported or invalid terrain file."); return false; }
	TArray<uint8> Snapshot;
	if (!FBase64::Decode(Save.DataBase64, Snapshot)) { OutError = TEXT("Terrain data is not valid Base64."); return false; }
	FChopItHeightField Candidate;
	if (!FChopItTerrainDataCodec::Decode(Snapshot, Candidate, OutError)) return false;
	if (Candidate.GetSamplesX() != Save.TerrainSettings.SamplesX || Candidate.GetSamplesY() != Save.TerrainSettings.SamplesY
		|| !FMath::IsNearlyEqual(Candidate.GetSize().X, Save.TerrainSettings.Width)
		|| !FMath::IsNearlyEqual(Candidate.GetSize().Y, Save.TerrainSettings.Length))
	{ OutError = TEXT("Terrain data does not match its settings."); return false; }
	if (Save.ManualStrokes.Num() > 4096)
	{ OutError = TEXT("Terrain file contains too many mask strokes."); return false; }
	for (const FChopItMaskStroke& Stroke : Save.ManualStrokes)
	{
		if (uint8(Stroke.Mask) > 3 || uint8(Stroke.Brush) > 2 || !FMath::IsFinite(Stroke.CenterUV.X)
			|| !FMath::IsFinite(Stroke.CenterUV.Y) || !FMath::IsFinite(Stroke.RadiusFraction)
			|| !FMath::IsFinite(Stroke.Strength) || !FMath::IsFinite(Stroke.TargetValue)
			|| Stroke.RadiusFraction <= 0 || Stroke.Strength < 0 || Stroke.Strength > 1
			|| Stroke.CenterUV.X < 0 || Stroke.CenterUV.X > 1 || Stroke.CenterUV.Y < 0 || Stroke.CenterUV.Y > 1
			|| Stroke.TargetValue < 0 || Stroke.TargetValue > 1)
		{ OutError = TEXT("Terrain file contains an invalid mask stroke."); return false; }
	}
	TArray<FChopItTerrainMeshChunk> ValidationMeshes;
	if (!FChopItTerrainMeshBuilder::Build(Candidate, Save.MeshSettings, ValidationMeshes, OutError)) return false;
	Modify();
	TerrainSettings = Save.TerrainSettings;
	MeshSettings = Save.MeshSettings;
	ManualStrokes = MoveTemp(Save.ManualStrokes);
	SnapshotStrokeCount = ManualStrokes.Num();
	SavedTerrainSettings = TerrainSettings;
	SavedTerrainData = MoveTemp(Snapshot);
	HeightField = MoveTemp(Candidate);
	PublishMeshes(ValidationMeshes, true);
	GeneratedSettings = TerrainSettings;
	GeneratedMeshSettings = MeshSettings;
	GeneratedMaterial = TerrainMaterial;
	MarkPackageDirty();
	return true;
}

bool AChopItTerrainActor::ExportTerrainMaps(const FString& DirectoryPath, FString& OutError) const
{
	if (!HeightField.IsValid() || !HeightField.GetMaps().IsValid(HeightField.GetHeights().Num()) || DirectoryPath.IsEmpty())
	{ OutError = TEXT("Terrain data or export directory is invalid."); return false; }
	if (!IFileManager::Get().MakeDirectory(*DirectoryPath, true))
	{ OutError = TEXT("Unable to create export directory."); return false; }
	const int32 X = HeightField.GetSamplesX(), Y = HeightField.GetSamplesY(), Count = X * Y;
	auto SaveMaskPNG = [&](const FString& Name, const uint8* Gray) -> bool
	{
		TArray<FColor> Pixels;
		Pixels.Reserve(Count);
		for (int32 I = 0; I < Count; ++I) Pixels.Add(FColor(Gray[I], Gray[I], Gray[I], 255));
		TArray64<uint8> Compressed;
		FImageUtils::PNGCompressImageArray(X, Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Compressed);
		if (Compressed.IsEmpty()) return false;
		TArray<uint8> Bytes;
		Bytes.Append(Compressed.GetData(), Compressed.Num());
		return FFileHelper::SaveArrayToFile(Bytes, *FPaths::Combine(DirectoryPath, Name + TEXT(".png")));
	};
	const float Minimum = HeightField.GetMinHeight(), Maximum = HeightField.GetMaxHeight();
	TArray<uint8> HeightBytes;
	HeightBytes.SetNumUninitialized(Count * 2);
	for (int32 I = 0; I < Count; ++I)
	{
		const uint16 Value = uint16(FMath::RoundToInt(65535.0 * FMath::Clamp((HeightField.GetHeights()[I] - Minimum)
			/ FMath::Max(Maximum - Minimum, UE_SMALL_NUMBER), 0.0f, 1.0f)));
		HeightBytes[I * 2] = uint8(Value); HeightBytes[I * 2 + 1] = uint8(Value >> 8);
	}
	if (!FFileHelper::SaveArrayToFile(HeightBytes, *FPaths::Combine(DirectoryPath, TEXT("Height.r16"))))
	{ OutError = TEXT("Unable to export heightmap."); return false; }
	struct FLayer { const TCHAR* Name; const TArray<float>* Values; bool bNormalized; };
	const FChopItTerrainMaps& Maps = HeightField.GetMaps();
	const FLayer ExportLayers[] = {
		{ TEXT("Slope"), &Maps.SlopeDegrees, false }, { TEXT("Curvature"), &Maps.Curvature, false },
		{ TEXT("Walkability"), &Maps.Walkability, true }, { TEXT("Clearing"), &Maps.Clearing, true },
		{ TEXT("Moisture"), &Maps.MoisturePotential, true }, { TEXT("VegetationDensity"), &Maps.VegetationDensityPotential, true }
	};
	FString Metadata = FString::Printf(TEXT("Width=%d\nHeight=%d\nHeightMinCm=%.9g\nHeightMaxCm=%.9g\nHeightFormat=R16_LE_RowMajor\nMaskFormat=PGM_P5_RowMajor\n"), X, Y, Minimum, Maximum);
	const FVector Origin = GetActorLocation(), Scale = GetActorScale3D();
	const FRotator Rotation = GetActorRotation();
	Metadata += FString::Printf(TEXT("SizeXcm=%.9g\nSizeYcm=%.9g\nWaterLevelCm=%.9g\nWorldLocationCm=%.9g,%.9g,%.9g\nWorldRotationDeg=%.9g,%.9g,%.9g\nWorldScale=%.9g,%.9g,%.9g\n"),
		HeightField.GetSize().X, HeightField.GetSize().Y, HeightField.GetWaterLevel(), Origin.X, Origin.Y, Origin.Z,
		Rotation.Pitch, Rotation.Yaw, Rotation.Roll, Scale.X, Scale.Y, Scale.Z);
	for (const FLayer& Layer : ExportLayers)
	{
		float Low = Layer.bNormalized ? 0.0f : TNumericLimits<float>::Max();
		float High = Layer.bNormalized ? 1.0f : TNumericLimits<float>::Lowest();
		if (!Layer.bNormalized) for (float Value : *Layer.Values) { Low = FMath::Min(Low, Value); High = FMath::Max(High, Value); }
		FTCHARToUTF8 Header(*FString::Printf(TEXT("P5\n%d %d\n255\n"), X, Y));
		TArray<uint8> Bytes;
		Bytes.Append(reinterpret_cast<const uint8*>(Header.Get()), Header.Length());
		for (float Value : *Layer.Values)
			Bytes.Add(uint8(FMath::RoundToInt(255.0f * FMath::Clamp((Value - Low) / FMath::Max(High - Low, UE_SMALL_NUMBER), 0.0f, 1.0f))));
		if (!FFileHelper::SaveArrayToFile(Bytes, *FPaths::Combine(DirectoryPath, FString(Layer.Name) + TEXT(".pgm"))))
		{ OutError = FString::Printf(TEXT("Unable to export %s mask."), Layer.Name); return false; }
		if (!SaveMaskPNG(Layer.Name, Bytes.GetData() + Bytes.Num() - Count))
		{ OutError = FString::Printf(TEXT("Unable to export %s PNG."), Layer.Name); return false; }
		Metadata += FString::Printf(TEXT("%sMin=%.9g\n%sMax=%.9g\n"), Layer.Name, Low, Layer.Name, High);
	}
	const TCHAR* RegionNames[] = { TEXT("FlatForest"), TEXT("RollingHills"), TEXT("SteepHills"), TEXT("Valleys"),
		TEXT("Lowlands"), TEXT("Clearings"), TEXT("Transitional") };
	const FString PgmHeader = FString::Printf(TEXT("P5\n%d %d\n255\n"), X, Y);
	FTCHARToUTF8 HeaderUtf8(*PgmHeader);
	for (int32 RegionIndex = -1; RegionIndex < FChopItTerrainRegionBlend::Count; ++RegionIndex)
	{
		TArray<uint8> Bytes;
		Bytes.Append(reinterpret_cast<const uint8*>(HeaderUtf8.Get()), HeaderUtf8.Length());
		for (int32 I = 0; I < Count; ++I)
		{
			Bytes.Add(RegionIndex < 0 ? uint8(Maps.Regions[I])
				: uint8(FMath::RoundToInt(255.0f * FMath::Clamp(Maps.RegionBlends[I].Weights[RegionIndex], 0.0f, 1.0f))));
		}
		const FString Name = RegionIndex < 0 ? TEXT("RegionIDs") : FString::Printf(TEXT("Region_%s"), RegionNames[RegionIndex]);
		if (!FFileHelper::SaveArrayToFile(Bytes, *FPaths::Combine(DirectoryPath, Name + TEXT(".pgm"))))
		{ OutError = FString::Printf(TEXT("Unable to export %s mask."), *Name); return false; }
		if (!SaveMaskPNG(Name, Bytes.GetData() + Bytes.Num() - Count))
		{ OutError = FString::Printf(TEXT("Unable to export %s PNG."), *Name); return false; }
	}
	Metadata += TEXT("RegionIDs=0:FlatForest,1:RollingHills,2:SteepHills,3:Valleys,4:Lowlands,5:Clearings,6:Transitional\n");
	if (!FFileHelper::SaveStringToFile(Metadata, *FPaths::Combine(DirectoryPath, TEXT("TerrainExport.txt"))))
	{ OutError = TEXT("Unable to export metadata."); return false; }
	return true;
}
