#include "Terrain/ChopItTerrainAnalysis.h"
#include "Terrain/ChopItTerrainNoise.h"
#include "Async/ParallelFor.h"

bool FChopItTerrainMaps::IsValid(int32 SampleCount) const
{
	return SampleCount > 0 && SlopeDegrees.Num() == SampleCount && Curvature.Num() == SampleCount
		&& Walkability.Num() == SampleCount && Clearing.Num() == SampleCount && MoisturePotential.Num() == SampleCount
		&& VegetationDensityPotential.Num() == SampleCount && Regions.Num() == SampleCount && RegionBlends.Num() == SampleCount;
}

namespace
{
double Smooth(double Low, double High, double Value)
{
	const double T = FMath::Clamp((Value - Low) / (High - Low), 0.0, 1.0);
	return T * T * (3.0 - 2.0 * T);
}

double FirstDerivative(const TArray<float>& H, int32 Index, int32 Stride, int32 Coordinate, int32 Count, double Step)
{
	if (Count == 2) return (double(H[Index + (1 - Coordinate) * Stride]) - H[Index - Coordinate * Stride]) / Step;
	if (Coordinate == 0) return (-3.0 * H[Index] + 4.0 * H[Index + Stride] - H[Index + 2 * Stride]) / (2.0 * Step);
	if (Coordinate == Count - 1) return (3.0 * H[Index] - 4.0 * H[Index - Stride] + H[Index - 2 * Stride]) / (2.0 * Step);
	return (double(H[Index + Stride]) - H[Index - Stride]) / (2.0 * Step);
}

double SecondDerivative(const TArray<float>& H, int32 Index, int32 Stride, int32 Coordinate, int32 Count, double Step)
{
	if (Count < 3) return 0.0;
	const int32 Start = Index + (FMath::Clamp(Coordinate - 1, 0, Count - 3) - Coordinate) * Stride;
	return (double(H[Start]) - 2.0 * H[Start + Stride] + H[Start + 2 * Stride]) / (Step * Step);
}

double Bilinear(const TArray<float>& H, FIntPoint Samples, double X, double Y)
{
	X = FMath::Clamp(X, 0.0, double(Samples.X - 1));
	Y = FMath::Clamp(Y, 0.0, double(Samples.Y - 1));
	const int32 IX = FMath::Min(FMath::FloorToInt(X), Samples.X - 2);
	const int32 IY = FMath::Min(FMath::FloorToInt(Y), Samples.Y - 2);
	const int32 I = IX + IY * Samples.X;
	return FMath::Lerp(FMath::Lerp(double(H[I]), double(H[I + 1]), X - IX),
		FMath::Lerp(double(H[I + Samples.X]), double(H[I + Samples.X + 1]), X - IX), Y - IY);
}
}

bool FChopItTerrainAnalysis::Build(const TArray<float>& Heights, FIntPoint Samples, FVector2D Size, float WaterLevel,
	int32 Seed, const FChopItTerrainRegionSettings& Settings, FChopItTerrainMaps& OutMaps, FString& OutError)
{
	if (!Settings.Validate(OutError)) return false;
	if (Samples.X < 2 || Samples.Y < 2 || Samples.X > 2049 || Samples.Y > 2049
		|| int64(Samples.X) * Samples.Y != Heights.Num() || !FMath::IsFinite(Size.X) || !FMath::IsFinite(Size.Y)
		|| Size.X < 100.0 || Size.Y < 100.0 || Size.X > 10000000.0 || Size.Y > 10000000.0 || !FMath::IsFinite(WaterLevel))
	{
		OutError = TEXT("Surface analysis requires a finite, correctly sized height grid and physical dimensions.");
		return false;
	}
	double Min = TNumericLimits<double>::Max(), Max = TNumericLimits<double>::Lowest();
	for (float H : Heights)
	{
		if (!FMath::IsFinite(H) || FMath::Abs(H) > 10000000.0f)
		{
			OutError = TEXT("Surface analysis requires finite elevations within +/-10000000 cm.");
			return false;
		}
		Min = FMath::Min(Min, double(H)); Max = FMath::Max(Max, double(H));
	}
	FChopItTerrainMaps Result;
	const int32 Count = Heights.Num();
	Result.SlopeDegrees.SetNumUninitialized(Count); Result.Curvature.SetNumUninitialized(Count);
	Result.Walkability.SetNumUninitialized(Count); Result.Clearing.SetNumUninitialized(Count);
	Result.MoisturePotential.SetNumUninitialized(Count); Result.VegetationDensityPotential.SetNumUninitialized(Count);
	Result.Regions.SetNumUninitialized(Count); Result.RegionBlends.SetNum(Count);
	const double StepX = Size.X / (Samples.X - 1), StepY = Size.Y / (Samples.Y - 1);
	const double RadiusX = Settings.RegionScale * 0.35 / StepX, RadiusY = Settings.RegionScale * 0.35 / StepY;
	const double Transition = Settings.TransitionWidth;
	const uint32 RegionSeed = ChopItTerrain::Mix(uint32(Seed) ^ 0x8c37ba59u);
	ParallelFor(Samples.Y, [&](int32 Y)
	{
		for (int32 X = 0; X < Samples.X; ++X)
		{
			const int32 I = X + Y * Samples.X;
			const double H = Heights[I];
			const double DX = FirstDerivative(Heights, I, 1, X, Samples.X, StepX);
			const double DY = FirstDerivative(Heights, I, Samples.X, Y, Samples.Y, StepY);
			const double Slope = FMath::RadiansToDegrees(FMath::Atan(FMath::Sqrt(DX * DX + DY * DY)));
			Result.SlopeDegrees[I] = float(Slope);
			Result.Curvature[I] = float(SecondDerivative(Heights, I, 1, X, Samples.X, StepX)
				+ SecondDerivative(Heights, I, Samples.X, Y, Samples.Y, StepY));
			// Symmetric broad samples avoid creating artificial valleys on sloping map borders.
			const double RX = FMath::Min(RadiusX, double(FMath::Min(X, Samples.X - 1 - X)));
			const double RY = FMath::Min(RadiusY, double(FMath::Min(Y, Samples.Y - 1 - Y)));
			const double BroadMean = (Bilinear(Heights, Samples, X - RX, Y) + Bilinear(Heights, Samples, X + RX, Y)
				+ Bilinear(Heights, Samples, X, Y - RY) + Bilinear(Heights, Samples, X, Y + RY)) * 0.25;
			const double Concavity = Max > Min ? FMath::Max(0.0, (BroadMean - H) / (Max - Min)) : 0.0;
			const double Valley = Smooth(0.015, 0.015 + Transition * 0.5, Concavity);
			const double Dry = Smooth(0.0, Settings.ShoreBlendHeight, H - WaterLevel);
			const double Walkable = Dry * (1.0 - Smooth(Settings.MaxWalkableSlopeDegrees * (1.0 - Transition),
				Settings.MaxWalkableSlopeDegrees * (1.0 + Transition), Slope));
			const double NX = (X * StepX - Size.X * 0.5) / Settings.RegionScale;
			const double NY = (Y * StepY - Size.Y * 0.5) / Settings.RegionScale;
			const double Selection = FMath::Clamp(0.5 + 1.5 * ChopItTerrain::Noise(NX + 27.39, NY - 18.17, RegionSeed), 0.0, 1.0);
			const double Coverage = Settings.ClearingCoverage;
			const double ClearingSelection = Coverage <= 0 ? 0.0 : Coverage >= 1 ? 1.0
				: Smooth(1.0 - Coverage - Transition, 1.0 - Coverage + Transition, Selection);
			const double Clearing = ClearingSelection * Dry * (1.0 - Smooth(Settings.FlatSlopeDegrees, Settings.SteepSlopeDegrees, Slope));
			const double WaterProximity = 1.0 - Smooth(0.0, Settings.MoistureHeightRange, H - WaterLevel);
			const double Moisture = WaterProximity + (1.0 - WaterProximity) * 0.3 * Valley;
			Result.Walkability[I] = float(Walkable);
			Result.Clearing[I] = float(Clearing);
			Result.MoisturePotential[I] = float(Moisture);
			Result.VegetationDensityPotential[I] = float(Settings.VegetationDensity * Walkable * (1.0 - Clearing) * (0.4 + 0.6 * Moisture));
			const double Flat = 1.0 - Smooth(Settings.FlatSlopeDegrees * (1.0 - Transition), Settings.FlatSlopeDegrees * (1.0 + Transition), Slope);
			const double Steep = Smooth(Settings.SteepSlopeDegrees * (1.0 - Transition), Settings.SteepSlopeDegrees * (1.0 + Transition), Slope);
			FChopItTerrainRegionBlend& Blend = Result.RegionBlends[I];
			Blend.Weights[int32(EChopItTerrainRegion::Clearings)] = float(Clearing);
			Blend.Weights[int32(EChopItTerrainRegion::Lowlands)] = float((1.0 - Clearing) * (1.0 - Dry));
			const double Remaining = (1.0 - Clearing) * Dry;
			Blend.Weights[int32(EChopItTerrainRegion::Valleys)] = float(Remaining * Valley);
			const double Upland = Remaining * (1.0 - Valley);
			Blend.Weights[int32(EChopItTerrainRegion::FlatForest)] = float(Upland * Flat);
			Blend.Weights[int32(EChopItTerrainRegion::SteepHills)] = float(Upland * (1.0 - Flat) * Steep);
			Blend.Weights[int32(EChopItTerrainRegion::RollingHills)] = float(Upland * (1.0 - Flat) * (1.0 - Steep));
			float Maximum = 0.0f;
			for (float W : Blend.Weights) Maximum = FMath::Max(Maximum, W);
			const double Transitional = 1.0 - Smooth(0.5, 0.5 + Transition, Maximum);
			for (float& W : Blend.Weights) W *= float(1.0 - Transitional);
			Blend.Weights[int32(EChopItTerrainRegion::Transitional)] = float(Transitional);
			int32 Dominant = 0;
			for (int32 R = 1; R < FChopItTerrainRegionBlend::Count; ++R)
				if (Blend.Weights[R] > Blend.Weights[Dominant]) Dominant = R;
			Result.Regions[I] = EChopItTerrainRegion(Dominant);
		}
	}, Samples.Y < 129 ? EParallelForFlags::ForceSingleThread : EParallelForFlags::None);
	OutMaps = MoveTemp(Result);
	return true;
}
