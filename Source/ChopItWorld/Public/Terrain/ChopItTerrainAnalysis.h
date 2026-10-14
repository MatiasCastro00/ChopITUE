#pragma once

#include "Terrain/ChopItTerrainMaps.h"
#include "Terrain/ChopItTerrainRegionSettings.h"

/** Pure CPU surface analysis; no actors, mesh resolution or editor state enter these calculations. */
class CHOPITWORLD_API FChopItTerrainAnalysis
{
public:
	static constexpr int32 AnalysisVersion = 1;
	/** Sizes and elevations are in cm. Invalid input preserves OutMaps. */
	static bool Build(const TArray<float>& Heights, FIntPoint Samples, FVector2D Size, float WaterLevel,
		int32 Seed, const FChopItTerrainRegionSettings& Settings, FChopItTerrainMaps& OutMaps, FString& OutError);
};
