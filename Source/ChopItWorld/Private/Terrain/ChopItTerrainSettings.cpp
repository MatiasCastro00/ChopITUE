#include "Terrain/ChopItTerrainSettings.h"

bool FChopItTerrainSettings::Validate(FString& OutError) const
{
	OutError.Reset();
	if (SamplesX < 2 || SamplesX > 2049 || SamplesY < 2 || SamplesY > 2049)
	{
		OutError = TEXT("Heightmap sample counts must be between 2 and 2049 per axis.");
		return false;
	}
	if (!FMath::IsFinite(Width) || !FMath::IsFinite(Length) || Width < 100.0 || Length < 100.0 || Width > 10000000.0 || Length > 10000000.0)
	{
		OutError = TEXT("Terrain dimensions must be finite and between 100 and 10000000 cm.");
		return false;
	}
	if (!FMath::IsFinite(SpawnFlatCenter.X) || !FMath::IsFinite(SpawnFlatCenter.Y)
		|| !FMath::IsFinite(SpawnFlatRadius) || SpawnFlatRadius < 0.0f || SpawnFlatRadius > 10000000.0f
		|| !FMath::IsFinite(SpawnFlatBlendWidth) || SpawnFlatBlendWidth < 0.0f || SpawnFlatBlendWidth > 10000000.0f
		|| (SpawnFlatRadius > 0.0f && (FMath::Abs(SpawnFlatCenter.X) > Width * 0.5 || FMath::Abs(SpawnFlatCenter.Y) > Length * 0.5)))
	{
		OutError = TEXT("The flat spawn area needs finite dimensions and a center inside the terrain.");
		return false;
	}
	if (!FMath::IsFinite(MinHeight) || !FMath::IsFinite(MaxHeight) || !FMath::IsFinite(TargetMeanHeight)
		|| FMath::Abs(MinHeight) > 10000000.0f || FMath::Abs(MaxHeight) > 10000000.0f
		|| MinHeight > MaxHeight || TargetMeanHeight < MinHeight || TargetMeanHeight > MaxHeight)
	{
		OutError = TEXT("Height bounds must be finite, ordered, within +/-10000000 cm and contain the target mean.");
		return false;
	}
	if (!FMath::IsFinite(HeightVariation) || HeightVariation < 0.0f || HeightVariation > 100000.0f
		|| !FMath::IsFinite(ReliefScale) || ReliefScale < 100.0 || ReliefScale > 10000000.0)
	{
		OutError = TEXT("Height variation must be in [0,100000] cm and relief scale in [100,10000000] cm.");
		return false;
	}
	if (!FMath::IsFinite(PlainsAmount) || PlainsAmount < 0.0f || PlainsAmount > 1.0f
		|| !FMath::IsFinite(HillStrength) || HillStrength < 0.0f || HillStrength > 2.0f
		|| !FMath::IsFinite(ValleyStrength) || ValleyStrength < 0.0f || ValleyStrength > 2.0f
		|| !FMath::IsFinite(SmallIrregularity) || SmallIrregularity < 0.0f || SmallIrregularity > 0.5f)
	{
		OutError = TEXT("Relief strengths are outside their supported ranges.");
		return false;
	}
	if (Octaves < 1 || Octaves > 8 || !FMath::IsFinite(Persistence) || Persistence < 0.0f || Persistence > 1.0f
		|| !FMath::IsFinite(Lacunarity) || Lacunarity < 1.0f || Lacunarity > 4.0f)
	{
		OutError = TEXT("Noise requires 1-8 octaves, persistence in [0,1] and lacunarity in [1,4].");
		return false;
	}
	if (NoiseLayers.Num() > 4)
	{
		OutError = TEXT("At most four independent noise layers are supported.");
		return false;
	}
	for (const FChopItTerrainNoiseLayer& Layer : NoiseLayers)
	{
		if (!FMath::IsFinite(Layer.Strength) || Layer.Strength < 0.0f || Layer.Strength > 2.0f
			|| !FMath::IsFinite(Layer.Frequency) || Layer.Frequency < 0.1f || Layer.Frequency > 32.0f
			|| Layer.Octaves < 1 || Layer.Octaves > 8
			|| !FMath::IsFinite(Layer.Persistence) || Layer.Persistence < 0.0f || Layer.Persistence > 1.0f
			|| !FMath::IsFinite(Layer.Lacunarity) || Layer.Lacunarity < 1.0f || Layer.Lacunarity > 4.0f)
		{
			OutError = TEXT("An independent noise layer has invalid strength, frequency or octave controls.");
			return false;
		}
	}
	if (!FMath::IsFinite(HillShape) || HillShape < 0.5f || HillShape > 4.0f
		|| !FMath::IsFinite(ValleyShape) || ValleyShape < 0.5f || ValleyShape > 4.0f
		|| !FMath::IsFinite(PlainsCoverage) || PlainsCoverage < 0.0f || PlainsCoverage > 1.0f
		|| !FMath::IsFinite(SlopeBias) || SlopeBias < -1.0f || SlopeBias > 1.0f
		|| !FMath::IsFinite(RoughnessFrequency) || RoughnessFrequency < 0.25f || RoughnessFrequency > 8.0f)
	{
		OutError = TEXT("Hill, valley, plain, slope or roughness controls are outside their supported ranges.");
		return false;
	}
	if (!FMath::IsFinite(DomainWarpStrength) || DomainWarpStrength < 0.0f || DomainWarpStrength > 2.0f
		|| !FMath::IsFinite(DomainWarpFrequency) || DomainWarpFrequency < 0.1f || DomainWarpFrequency > 8.0f
		|| !FMath::IsFinite(EdgeFalloff) || EdgeFalloff < 0.0f || EdgeFalloff > 1.0f
		|| !FMath::IsFinite(EdgeFalloffStart) || EdgeFalloffStart < 0.0f || EdgeFalloffStart > 0.95f
		|| !FMath::IsFinite(DepressionStrength) || DepressionStrength < 0.0f || DepressionStrength > 2.0f
		|| !FMath::IsFinite(DepressionFrequency) || DepressionFrequency < 0.1f || DepressionFrequency > 8.0f)
	{
		OutError = TEXT("Warp, edge or depression controls are outside their supported ranges.");
		return false;
	}
	if (HeightDistributionCurve.Num() > 16)
	{
		OutError = TEXT("The height curve supports at most sixteen points.");
		return false;
	}
	if (!HeightDistributionCurve.IsEmpty())
	{
		if (HeightDistributionCurve.Num() < 2 || HeightDistributionCurve[0] != FVector2D::ZeroVector
			|| HeightDistributionCurve.Last() != FVector2D(1.0, 1.0))
		{
			OutError = TEXT("The height curve must start at (0,0) and end at (1,1).");
			return false;
		}
		for (int32 Index = 0; Index < HeightDistributionCurve.Num(); ++Index)
		{
			const FVector2D Point = HeightDistributionCurve[Index];
			if (!FMath::IsFinite(Point.X) || !FMath::IsFinite(Point.Y) || Point.X < 0.0 || Point.X > 1.0
				|| Point.Y < 0.0 || Point.Y > 1.0
				|| (Index > 0 && (Point.X <= HeightDistributionCurve[Index - 1].X || Point.Y < HeightDistributionCurve[Index - 1].Y)))
			{
				OutError = TEXT("Height curve points must have increasing X and nondecreasing Y in [0,1].");
				return false;
			}
		}
	}
	if (!FMath::IsFinite(TerraceStrength) || TerraceStrength < 0.0f || TerraceStrength > 1.0f
		|| TerraceCount < 2 || TerraceCount > 64
		|| !FMath::IsFinite(ErosionStrength) || ErosionStrength < 0.0f || ErosionStrength > 1.0f
		|| ErosionIterations < 1 || ErosionIterations > 8)
	{
		OutError = TEXT("Terrace or erosion controls are outside their supported ranges.");
		return false;
	}
	if (!FMath::IsFinite(WaterLevel) || FMath::Abs(WaterLevel) > 10000000.0f
		|| !FMath::IsFinite(TargetSubmergedFraction) || TargetSubmergedFraction < 0.0f || TargetSubmergedFraction > 1.0f)
	{
		OutError = TEXT("Water level or target submerged fraction is invalid.");
		return false;
	}
	return Regions.Validate(OutError);
}
