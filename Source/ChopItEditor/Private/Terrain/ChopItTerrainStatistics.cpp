#include "Terrain/ChopItTerrainStatistics.h"
#include "Terrain/ChopItTerrainActor.h"

FString FChopItTerrainStatistics::Describe(const AChopItTerrainActor* Actor)
{
	if (!IsValid(Actor) || !Actor->GetHeightField().IsValid()) return TEXT("No generated terrain data.");
	const FChopItHeightField& Field = Actor->GetHeightField();
	const FChopItTerrainMaps& Maps = Field.GetMaps();
	const int32 Count = Field.GetHeights().Num();
	FString Result = FString::Printf(TEXT("Grid %d x %d (%d samples) | Size %.0f x %.0f cm | Chunks %d | Manual strokes %d\n"),
		Field.GetSamplesX(), Field.GetSamplesY(), Count, Field.GetSize().X, Field.GetSize().Y,
		Actor->GetTerrainChunks().Num(), Actor->GetManualStrokeCount());
	Result += FString::Printf(TEXT("Elevation %.1f to %.1f cm | Mean %.1f cm | Water %.1f cm | Submerged %.1f%%\n"),
		Field.GetMinHeight(), Field.GetMaxHeight(), Field.GetMeanHeight(), Field.GetWaterLevel(), Field.GetSubmergedFraction() * 100.0);
	if (!Maps.IsValid(Count)) return Result;
	double Walk = 0, Clearing = 0, Moisture = 0, Density = 0;
	int32 Regions[FChopItTerrainRegionBlend::Count] = {};
	for (int32 I = 0; I < Count; ++I)
	{
		Walk += Maps.Walkability[I]; Clearing += Maps.Clearing[I];
		Moisture += Maps.MoisturePotential[I]; Density += Maps.VegetationDensityPotential[I];
		const int32 Region = int32(Maps.Regions[I]);
		if (Region >= 0 && Region < FChopItTerrainRegionBlend::Count) ++Regions[Region];
	}
	Result += FString::Printf(TEXT("Mean masks: walkable %.0f%% | clearing %.0f%% | moisture %.0f%% | vegetation %.0f%%\n"),
		Walk * 100.0 / Count, Clearing * 100.0 / Count, Moisture * 100.0 / Count, Density * 100.0 / Count);
	Result += FString::Printf(TEXT("Regions: flat %.0f%% | rolling %.0f%% | steep %.0f%% | valleys %.0f%% | lowlands %.0f%% | clearings %.0f%% | transitional %.0f%%"),
		100.0 * Regions[0] / Count, 100.0 * Regions[1] / Count, 100.0 * Regions[2] / Count,
		100.0 * Regions[3] / Count, 100.0 * Regions[4] / Count, 100.0 * Regions[5] / Count,
		100.0 * Regions[6] / Count);
	return Result;
}
