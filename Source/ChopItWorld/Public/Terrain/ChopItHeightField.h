#pragma once

#include "CoreMinimal.h"
#include "Terrain/ChopItTerrainMaps.h"

class FChopItTerrainGenerator;
class AChopItTerrainActor;
class FChopItTerrainDataCodec;

/** Row-major samples: X + Y * SamplesX. Local XY is centered on the terrain. */
struct CHOPITWORLD_API FChopItHeightField
{
public:
	bool IsValid() const;
	int32 GetSamplesX() const { return SamplesX; }
	int32 GetSamplesY() const { return SamplesY; }
	FVector2D GetSize() const { return Size; }
	const TArray<float>& GetHeights() const { return Heights; }
	float GetMinHeight() const { return MinHeight; }
	float GetMaxHeight() const { return MaxHeight; }
	double GetMeanHeight() const { return MeanHeight; }
	float GetWaterLevel() const { return WaterLevel; }
	double GetSubmergedFraction() const { return SubmergedFraction; }
	const FChopItTerrainMaps& GetMaps() const { return Maps; }

	/** Bilinear interpolation in local centimeters; outside points fail instead of clamping. */
	bool SampleLocalHeight(const FVector2D& LocalXY, float& OutHeight) const;
	bool SampleLocalMap(EChopItTerrainMap Map, const FVector2D& LocalXY, float& OutValue) const;
	bool SampleLocalRegionBlend(const FVector2D& LocalXY, FChopItTerrainRegionBlend& OutBlend) const;

private:
	friend class FChopItTerrainGenerator;
	friend class AChopItTerrainActor;
	friend class FChopItTerrainDataCodec;
	int32 SamplesX = 0;
	int32 SamplesY = 0;
	FVector2D Size = FVector2D::ZeroVector;
	TArray<float> Heights;
	float MinHeight = 0.0f;
	float MaxHeight = 0.0f;
	double MeanHeight = 0.0;
	float WaterLevel = 0.0f;
	double SubmergedFraction = 0.0;
	FChopItTerrainMaps Maps;
	bool GetSampleCoordinates(const FVector2D& LocalXY, int32& OutIndex, FVector2D& OutAlpha) const;
};
