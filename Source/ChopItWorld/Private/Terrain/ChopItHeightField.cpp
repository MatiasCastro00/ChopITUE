#include "Terrain/ChopItHeightField.h"

bool FChopItHeightField::IsValid() const
{
	return SamplesX >= 2 && SamplesY >= 2 && int64(SamplesX) * SamplesY == Heights.Num()
		&& FMath::IsFinite(Size.X) && FMath::IsFinite(Size.Y) && Size.X > 0.0 && Size.Y > 0.0;
}

bool FChopItHeightField::SampleLocalHeight(const FVector2D& LocalXY, float& OutHeight) const
{
	return SampleLocalMap(EChopItTerrainMap::Height, LocalXY, OutHeight);
}

bool FChopItHeightField::GetSampleCoordinates(const FVector2D& LocalXY, int32& OutIndex, FVector2D& OutAlpha) const
{
	if (!IsValid() || !FMath::IsFinite(LocalXY.X) || !FMath::IsFinite(LocalXY.Y)
		|| FMath::Abs(LocalXY.X) > Size.X * 0.5 || FMath::Abs(LocalXY.Y) > Size.Y * 0.5)
	{
		return false;
	}
	const double GridX = FMath::Clamp((LocalXY.X / Size.X + 0.5) * (SamplesX - 1), 0.0, double(SamplesX - 1));
	const double GridY = FMath::Clamp((LocalXY.Y / Size.Y + 0.5) * (SamplesY - 1), 0.0, double(SamplesY - 1));
	const int32 X0 = FMath::Min(FMath::FloorToInt(GridX), SamplesX - 2);
	const int32 Y0 = FMath::Min(FMath::FloorToInt(GridY), SamplesY - 2);
	OutIndex = X0 + Y0 * SamplesX;
	OutAlpha = FVector2D(GridX - X0, GridY - Y0);
	return true;
}

bool FChopItHeightField::SampleLocalMap(EChopItTerrainMap Map, const FVector2D& LocalXY, float& OutValue) const
{
	OutValue = 0.0f;
	const TArray<float>* Values = nullptr;
	switch (Map)
	{
	case EChopItTerrainMap::Height: Values = &Heights; break;
	case EChopItTerrainMap::Slope: Values = &Maps.SlopeDegrees; break;
	case EChopItTerrainMap::Curvature: Values = &Maps.Curvature; break;
	case EChopItTerrainMap::Walkability: Values = &Maps.Walkability; break;
	case EChopItTerrainMap::Clearing: Values = &Maps.Clearing; break;
	case EChopItTerrainMap::Moisture: Values = &Maps.MoisturePotential; break;
	case EChopItTerrainMap::VegetationDensity: Values = &Maps.VegetationDensityPotential; break;
	}
	int32 I; FVector2D Alpha;
	if (!Values || Values->Num() != Heights.Num() || !GetSampleCoordinates(LocalXY, I, Alpha)) return false;
	const double A = FMath::Lerp(double((*Values)[I]), double((*Values)[I + 1]), Alpha.X);
	const double B = FMath::Lerp(double((*Values)[I + SamplesX]), double((*Values)[I + SamplesX + 1]), Alpha.X);
	OutValue = float(FMath::Lerp(A, B, Alpha.Y));
	return true;
}

bool FChopItHeightField::SampleLocalRegionBlend(const FVector2D& LocalXY, FChopItTerrainRegionBlend& OutBlend) const
{
	OutBlend = FChopItTerrainRegionBlend();
	int32 I; FVector2D Alpha;
	if (Maps.RegionBlends.Num() != Heights.Num() || !GetSampleCoordinates(LocalXY, I, Alpha)) return false;
	for (int32 R = 0; R < FChopItTerrainRegionBlend::Count; ++R)
	{
		const double A = FMath::Lerp(double(Maps.RegionBlends[I].Weights[R]), double(Maps.RegionBlends[I + 1].Weights[R]), Alpha.X);
		const double B = FMath::Lerp(double(Maps.RegionBlends[I + SamplesX].Weights[R]), double(Maps.RegionBlends[I + SamplesX + 1].Weights[R]), Alpha.X);
		OutBlend.Weights[R] = float(FMath::Lerp(A, B, Alpha.Y));
	}
	return true;
}
