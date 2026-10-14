#include "Terrain/ChopItTerrainRegionSettings.h"

bool FChopItTerrainRegionSettings::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!FMath::IsFinite(RegionScale) || RegionScale < 100.0 || RegionScale > 10000000.0
		|| !FMath::IsFinite(ClearingCoverage) || ClearingCoverage < 0 || ClearingCoverage > 1
		|| !FMath::IsFinite(TransitionWidth) || TransitionWidth < 0.01f || TransitionWidth > 0.45f
		|| !FMath::IsFinite(FlatSlopeDegrees) || FlatSlopeDegrees < 0.1f || FlatSlopeDegrees > 85
		|| !FMath::IsFinite(SteepSlopeDegrees) || SteepSlopeDegrees <= FlatSlopeDegrees || SteepSlopeDegrees > 89
		|| !FMath::IsFinite(MaxWalkableSlopeDegrees) || MaxWalkableSlopeDegrees < 1 || MaxWalkableSlopeDegrees > 89
		|| !FMath::IsFinite(ShoreBlendHeight) || ShoreBlendHeight < 1 || ShoreBlendHeight > 100000
		|| !FMath::IsFinite(MoistureHeightRange) || MoistureHeightRange < 1 || MoistureHeightRange > 100000
		|| !FMath::IsFinite(VegetationDensity) || VegetationDensity < 0 || VegetationDensity > 1)
	{
		OutError = TEXT("Region settings require finite controls in range and FlatSlopeDegrees < SteepSlopeDegrees.");
		return false;
	}
	return true;
}
