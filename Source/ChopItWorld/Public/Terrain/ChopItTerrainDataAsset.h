#pragma once

#include "Engine/DataAsset.h"
#include "Terrain/ChopItTerrainSettings.h"
#include "Terrain/ChopItTerrainMeshSettings.h"
#include "ChopItTerrainDataAsset.generated.h"

class AChopItTerrainActor;

/** Serializable terrain controls without a seed. A run supplies its own seed. */
USTRUCT(BlueprintType)
struct CHOPITWORLD_API FChopItTerrainProfile
{
	GENERATED_BODY()

	FChopItTerrainProfile();
	void CopyFrom(const FChopItTerrainSettings& Source);
	FChopItTerrainSettings WithSeed(int32 Seed) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn") FVector2D SpawnFlatCenter;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn") float SpawnFlatRadius;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn") float SpawnFlatBlendWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Size") double Width;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Size") double Length;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Heightmap") int32 SamplesX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Heightmap") int32 SamplesY;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Height") float MinHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Height") float MaxHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Height") float TargetMeanHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Height") float HeightVariation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Relief") double ReliefScale;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Relief") float PlainsAmount;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Relief") float HillStrength;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Relief") float ValleyStrength;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Relief") float SmallIrregularity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise") int32 Octaves;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise") float Persistence;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise") float Lacunarity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise") TArray<FChopItTerrainNoiseLayer> NoiseLayers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hills") float HillShape;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Valleys") float ValleyShape;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Plains") float PlainsCoverage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slopes") float SlopeBias;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Roughness") float RoughnessFrequency;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp") float DomainWarpStrength;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp") float DomainWarpFrequency;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Edges") float EdgeFalloff;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Edges") float EdgeFalloffStart;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Height Curve") TArray<FVector2D> HeightDistributionCurve;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Depressions") float DepressionStrength;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Depressions") float DepressionFrequency;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terraces") float TerraceStrength;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terraces") int32 TerraceCount;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion") float ErosionStrength;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion") int32 ErosionIterations;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water") float WaterLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water") bool bAutoWaterLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water") float TargetSubmergedFraction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Regions") FChopItTerrainRegionSettings Regions;
};

/** Saved terrain recipe. The seed is supplied by the game when generating a world. */
UCLASS(BlueprintType)
class CHOPITWORLD_API UChopItTerrainDataAsset final : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Terrain")
	FChopItTerrainProfile Terrain;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Terrain")
	FChopItTerrainMeshSettings Mesh;

	UFUNCTION(BlueprintPure, Category="Terrain", meta=(ToolTip="Build settings using the seed chosen for this game run. The Data Asset has no stored seed."))
	FChopItTerrainSettings MakeSettings(int32 RuntimeSeed) const { return Terrain.WithSeed(RuntimeSeed); }

	/** Applies this recipe to an existing actor and rebuilds it with a seed chosen for this run. */
	UFUNCTION(BlueprintCallable, Category="Terrain", meta=(ToolTip="Generate this recipe on an existing terrain actor using the seed chosen for this game run."))
	bool GenerateOnActor(AChopItTerrainActor* Actor, int32 RuntimeSeed, FString& OutError) const;

	/** Spawn the terrain once with this recipe and a seed chosen for the current run. */
	UFUNCTION(BlueprintCallable, Category="Terrain", meta=(WorldContext="WorldContextObject", ToolTip="Spawn terrain from this recipe using a runtime seed. Returns null on invalid settings or generation failure."))
	AChopItTerrainActor* SpawnTerrain(const UObject* WorldContextObject, FTransform Transform,
		int32 RuntimeSeed, FString& OutError) const;

	/** C++ variant for worlds that choose a spawn position before generation. */
	AChopItTerrainActor* SpawnTerrainWithSettings(const UObject* WorldContextObject, FTransform Transform,
		const FChopItTerrainSettings& RunSettings, FString& OutError) const;
};
