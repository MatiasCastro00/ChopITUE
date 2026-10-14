#pragma once

#include "CoreMinimal.h"
#include "Terrain/ChopItTerrainRegionSettings.h"
#include "ChopItTerrainSettings.generated.h"

/** An optional, independently seeded noise field added to the main relief. */
USTRUCT(BlueprintType)
struct CHOPITWORLD_API FChopItTerrainNoiseLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Layer", meta=(ToolTip="Enable this additional deterministic noise layer."))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Layer", meta = (ClampMin = "0", ClampMax = "2", ToolTip="How strongly this layer contributes to the height field."))
	float Strength = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Layer", meta = (ClampMin = "0.1", ClampMax = "32", ToolTip = "Frequency relative to the main relief scale."))
	float Frequency = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Layer", meta = (ClampMin = "1", ClampMax = "8", ToolTip="Number of noise octaves in this layer."))
	int32 Octaves = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Layer", meta = (ClampMin = "0", ClampMax = "1", ToolTip="Amplitude retained by successive octaves in this layer."))
	float Persistence = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Layer", meta = (ClampMin = "1", ClampMax = "4", ToolTip="Frequency multiplier between this layer's octaves."))
	float Lacunarity = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Layer", meta = (ToolTip = "Independent deterministic seed offset for this layer."))
	int32 SeedOffset = 1;
};

/** Settings for a forest height field. Distances and heights are in centimeters. */
USTRUCT(BlueprintType)
struct CHOPITWORLD_API FChopItTerrainSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Generation", meta = (ToolTip = "Same seed, settings and generator version produce the same height field."))
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Spawn", meta=(ToolTip="Center of an optional level spawn area, in terrain-local centimeters."))
	FVector2D SpawnFlatCenter = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Spawn", meta=(ClampMin="0", Units="cm", ToolTip="Radius of the completely flat spawn area. Zero disables it."))
	float SpawnFlatRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Spawn", meta=(ClampMin="0", Units="cm", ToolTip="Smooth transition from the flat spawn area to the surrounding relief."))
	float SpawnFlatBlendWidth = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Size", meta = (ClampMin = "100", ClampMax = "10000000", UIMin = "1000", UIMax = "100000", Units = "cm", ToolTip="Terrain width along local X, in centimeters."))
	double Width = 20000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Size", meta = (ClampMin = "100", ClampMax = "10000000", UIMin = "1000", UIMax = "100000", Units = "cm", ToolTip="Terrain length along local Y, in centimeters."))
	double Length = 20000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Heightmap", meta = (ClampMin = "2", ClampMax = "2049", UIMin = "2", UIMax = "1025", ToolTip = "Number of height samples including both borders. Independent of future mesh polygon detail."))
	int32 SamplesX = 257;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Heightmap", meta = (ClampMin = "2", ClampMax = "2049", UIMin = "2", UIMax = "1025", ToolTip="Number of height samples along local Y, including both borders."))
	int32 SamplesY = 257;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Height", meta = (UIMin = "-3000", UIMax = "3000", Units = "cm", ToolTip = "Lower height bound, not a required minimum reached by the terrain."))
	float MinHeight = -300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Height", meta = (UIMin = "-3000", UIMax = "3000", Units = "cm", ToolTip = "Upper height bound, not a required maximum reached by the terrain."))
	float MaxHeight = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Height", meta = (UIMin = "-3000", UIMax = "3000", Units = "cm", ToolTip = "Target arithmetic mean of the heightmap samples. Must lie within the height bounds."))
	float TargetMeanHeight = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Height", meta = (ClampMin = "0", ClampMax = "100000", UIMin = "0", UIMax = "3000", Units = "cm", ToolTip = "Vertical amplitude before fitting the height bounds. Zero produces a flat field at the target mean."))
	float HeightVariation = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Relief", meta = (ClampMin = "100", ClampMax = "10000000", UIMin = "100", UIMax = "50000", Units = "cm", ToolTip = "Horizontal size of the main relief features. Larger values make broader hills and valleys."))
	double ReliefScale = 6000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Relief", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", ToolTip = "Strength of smooth flattening in noise-selected areas; not a coverage percentage."))
	float PlainsAmount = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Relief", meta = (ClampMin = "0", ClampMax = "2", UIMin = "0", UIMax = "2", ToolTip = "Weight of broad positive noise before fitting height bounds. Higher values make hills more prominent."))
	float HillStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Relief", meta = (ClampMin = "0", ClampMax = "2", UIMin = "0", UIMax = "2", ToolTip = "Weight of broad negative noise. Higher values deepen and emphasize valleys."))
	float ValleyStrength = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Relief", meta = (ClampMin = "0", ClampMax = "0.5", UIMin = "0", UIMax = "0.5", ToolTip = "Small surface irregularities relative to the main vertical amplitude."))
	float SmallIrregularity = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Noise", meta = (ClampMin = "1", ClampMax = "8", UIMin = "1", UIMax = "8", ToolTip = "Number of main noise octaves. More octaves add finer shapes with diminishing amplitude."))
	int32 Octaves = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Noise", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", ToolTip = "Amplitude retained by each successive noise octave."))
	float Persistence = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Noise", meta = (ClampMin = "1", ClampMax = "4", UIMin = "1", UIMax = "4", ToolTip = "Frequency multiplier between noise octaves."))
	float Lacunarity = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Noise", meta = (ToolTip = "Optional additive noise layers. Each has its own seed, frequency and octave controls; at most four layers."))
	TArray<FChopItTerrainNoiseLayer> NoiseLayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Hills", meta = (ClampMin = "0.5", ClampMax = "4", ToolTip = "Power applied to positive main noise. Lower values broaden hills; higher values concentrate peaks."))
	float HillShape = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Valleys", meta = (ClampMin = "0.5", ClampMax = "4", ToolTip = "Power applied to negative main noise. Higher values narrow valley floors."))
	float ValleyShape = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Plains", meta = (ClampMin = "0", ClampMax = "1", ToolTip = "Fraction of the broad plain-selection noise range covered by flattened terrain."))
	float PlainsCoverage = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Slopes", meta = (ClampMin = "-1", ClampMax = "1", ToolTip = "Negative smooths local slopes; positive sharpens them. Zero leaves them unchanged."))
	float SlopeBias = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Roughness", meta = (ClampMin = "0.25", ClampMax = "8", ToolTip = "Frequency of the small irregularities; their amplitude is SmallIrregularity."))
	float RoughnessFrequency = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Warp", meta = (ClampMin = "0", ClampMax = "2", ToolTip = "Displaces sample coordinates using two independent noise fields; zero disables domain warping."))
	float DomainWarpStrength = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Warp", meta = (ClampMin = "0.1", ClampMax = "8", ToolTip = "Scale of coordinate-warp noise; effective when Domain Warp Strength is above zero."))
	float DomainWarpFrequency = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Edges", meta = (ClampMin = "0", ClampMax = "1", ToolTip = "Fades raw relief toward zero at the terrain boundary, before global height fitting."))
	float EdgeFalloff = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Edges", meta = (ClampMin = "0", ClampMax = "0.95", ToolTip = "Normalized distance from center where edge falloff begins."))
	float EdgeFalloffStart = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Height Curve", meta = (ToolTip = "Optional normalized (input, output) points from (0,0) to (1,1), sorted and nondecreasing. Empty means identity."))
	TArray<FVector2D> HeightDistributionCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Depressions", meta = (ClampMin = "0", ClampMax = "2", ToolTip = "Depth of smooth local depressions relative to the main relief."))
	float DepressionStrength = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Depressions", meta = (ClampMin = "0.1", ClampMax = "8"))
	float DepressionFrequency = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Terraces", meta = (ClampMin = "0", ClampMax = "1", ToolTip = "Blend between continuous heights and terraced heights; zero disables terraces."))
	float TerraceStrength = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Terraces", meta = (ClampMin = "2", ClampMax = "64"))
	int32 TerraceCount = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Erosion", meta = (ClampMin = "0", ClampMax = "1", ToolTip = "Light mass-conserving thermal erosion; zero disables it."))
	float ErosionStrength = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Erosion", meta = (ClampMin = "1", ClampMax = "8"))
	int32 ErosionIterations = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Water", meta = (UIMin = "-3000", UIMax = "3000", Units = "cm", ToolTip = "Manual water elevation. Used directly when automatic coverage is disabled."))
	float WaterLevel = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Water", meta = (ToolTip = "Choose water elevation from the generated height distribution to match target submerged coverage."))
	bool bAutoWaterLevel = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Water", meta = (ClampMin = "0", ClampMax = "1", ToolTip = "Target submerged XY area fraction estimated from equal-area heightmap cell centers when automatic water level is enabled."))
	float TargetSubmergedFraction = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terrain|Regions", meta=(ToolTip="Regional classification, clearing and vegetation suitability controls."))
	FChopItTerrainRegionSettings Regions;

	/** Runtime validation also applies to settings constructed directly in C++. */
	bool Validate(FString& OutError) const;
};
