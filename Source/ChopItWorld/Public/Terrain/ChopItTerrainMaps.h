#pragma once

#include "CoreMinimal.h"
#include "ChopItTerrainMaps.generated.h"

UENUM(BlueprintType)
enum class EChopItTerrainRegion : uint8
{
	FlatForest, RollingHills, SteepHills, Valleys, Lowlands, Clearings, Transitional, Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EChopItTerrainMap : uint8
{
	Height, Slope, Curvature, Walkability, Clearing, Moisture, VegetationDensity
};

UENUM(BlueprintType)
enum class EChopItEditableMask : uint8
{
	Walkability, Clearing, Moisture, VegetationDensity
};

UENUM(BlueprintType)
enum class EChopItMaskBrush : uint8
{
	Paint, Erase, Smooth
};

/** Brush coordinates and radius are normalized to the terrain, so edits survive size changes. */
USTRUCT(BlueprintType)
struct CHOPITWORLD_API FChopItMaskStroke
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask")
	EChopItEditableMask Mask = EChopItEditableMask::Clearing;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask")
	EChopItMaskBrush Brush = EChopItMaskBrush::Paint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask")
	FVector2D CenterUV = FVector2D(0.5, 0.5);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask", meta = (ClampMin = "0.0001", ClampMax = "1"))
	float RadiusFraction = 0.05f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask", meta = (ClampMin = "0", ClampMax = "1"))
	float Strength = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask", meta = (ClampMin = "0", ClampMax = "1"))
	float TargetValue = 1.0f;
};

/** Continuous memberships sum to one. The Region Map stores their dominant category. */
struct FChopItTerrainRegionBlend
{
	static constexpr int32 Count = int32(EChopItTerrainRegion::Count);
	float Weights[Count] = {};
};

/** Same row-major sample lattice as the heightmap. Potential masks are in [0,1]. */
struct CHOPITWORLD_API FChopItTerrainMaps
{
	TArray<float> SlopeDegrees;
	/** Laplacian of elevation, in 1/cm. Positive is concave, negative is convex. */
	TArray<float> Curvature;
	TArray<float> Walkability;
	TArray<float> Clearing;
	TArray<float> MoisturePotential;
	TArray<float> VegetationDensityPotential;
	TArray<EChopItTerrainRegion> Regions;
	TArray<FChopItTerrainRegionBlend> RegionBlends;

	bool IsValid(int32 SampleCount) const;
};
