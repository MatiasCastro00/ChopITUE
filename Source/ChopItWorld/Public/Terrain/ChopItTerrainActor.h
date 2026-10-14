#pragma once

#include "GameFramework/Actor.h"
#include "Terrain/ChopItHeightField.h"
#include "Terrain/ChopItTerrainSettings.h"
#include "Terrain/ChopItTerrainMeshSettings.h"
#include "ChopItTerrainActor.generated.h"

class UDynamicMeshComponent;
class UMaterialInterface;
struct FChopItTerrainMeshChunk;

USTRUCT()
struct FChopItTerrainSaveFile
{
	GENERATED_BODY()
	UPROPERTY() int32 Version = 1;
	UPROPERTY() FChopItTerrainSettings TerrainSettings;
	UPROPERTY() FChopItTerrainMeshSettings MeshSettings;
	UPROPERTY() TArray<FChopItMaskStroke> ManualStrokes;
	UPROPERTY() FString DataBase64;
};

/** Placeable terrain. Regeneration updates owned components; never spawns another actor. */
UCLASS(Blueprintable)
class CHOPITWORLD_API AChopItTerrainActor : public AActor
{
	GENERATED_BODY()

public:
	AChopItTerrainActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostLoad() override;
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	FChopItTerrainSettings TerrainSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	FChopItTerrainMeshSettings MeshSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	TObjectPtr<UMaterialInterface> TerrainMaterial;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Terrain")
	void RegenerateTerrain();

	/** Must run on the game thread. Invalid settings preserve the previous geometry and heightmap. */
	bool RebuildTerrain(FString& OutError);
	/** Query at the actor's local XY projected from a world position. OutHeight is world Z. */
	UFUNCTION(BlueprintCallable, Category = "Terrain|Data")
	bool SampleWorldHeight(FVector WorldPosition, float& OutHeight) const;
	/** Normal of the bilinear source heightfield, transformed with inverse scale. */
	UFUNCTION(BlueprintCallable, Category = "Terrain|Data")
	bool SampleWorldNormal(FVector WorldPosition, FVector& OutNormal) const;
	UFUNCTION(BlueprintCallable, Category = "Terrain|Data")
	bool SampleWorldMap(EChopItTerrainMap Map, FVector WorldPosition, float& OutValue) const;
	UFUNCTION(BlueprintCallable, Category = "Terrain|Data")
	bool SampleWorldRegion(FVector WorldPosition, EChopItTerrainRegion& OutRegion) const;
	UFUNCTION(BlueprintCallable, Category = "Terrain|Data")
	bool SampleWorldRegionWeights(FVector WorldPosition, TArray<float>& OutWeights) const;
	UFUNCTION(BlueprintCallable, Category = "Terrain|Masks")
	bool ApplyWorldMaskBrush(EChopItEditableMask Mask, EChopItMaskBrush Brush, FVector WorldPosition,
		float RadiusCm, float Strength, float TargetValue, FString& OutError);
	/** Versioned terrain snapshot, including configuration and manual mask strokes. */
	UFUNCTION(BlueprintCallable, Category = "Terrain|Data")
	bool SaveTerrainData(const FString& FilePath, FString& OutError) const;
	UFUNCTION(BlueprintCallable, Category = "Terrain|Data")
	bool LoadTerrainData(const FString& FilePath, FString& OutError);
	UFUNCTION(BlueprintCallable, Category = "Terrain|Data")
	bool ExportTerrainMaps(const FString& DirectoryPath, FString& OutError) const;
	UFUNCTION(BlueprintPure, Category = "Terrain|Masks")
	int32 GetManualStrokeCount() const { return ManualStrokes.Num(); }
	const FChopItHeightField& GetHeightField() const { return HeightField; }
	const FChopItTerrainSettings& GetGeneratedSettings() const { return GeneratedSettings; }
	const TArray<TObjectPtr<UDynamicMeshComponent>>& GetTerrainChunks() const { return TerrainChunks; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Terrain")
	TArray<TObjectPtr<UDynamicMeshComponent>> TerrainChunks;
	UPROPERTY()
	TArray<uint8> SavedTerrainData;
	UPROPERTY()
	FChopItTerrainSettings SavedTerrainSettings;
	UPROPERTY()
	TArray<FChopItMaskStroke> ManualStrokes;
	/** -1 identifies legacy snapshots which already contain every saved stroke. */
	UPROPERTY()
	int32 SnapshotStrokeCount = -1;

	FChopItHeightField HeightField;
	bool RestoreSnapshot(FChopItHeightField& OutField, FString& OutError) const;
	void PublishMeshes(TArray<FChopItTerrainMeshChunk>& Meshes, bool bReplaceGeometry);
	bool ApplyStroke(FChopItHeightField& Field, const FChopItHeightField& BaseField, const FChopItMaskStroke& Stroke) const;
	FChopItTerrainSettings GeneratedSettings;
	FChopItTerrainMeshSettings GeneratedMeshSettings;
	TWeakObjectPtr<UMaterialInterface> GeneratedMaterial;
};
