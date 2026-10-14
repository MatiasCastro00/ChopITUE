#include "Terrain/ChopItTerrainGenerator.h"
#include "Terrain/ChopItTerrainNoise.h"
#include "Terrain/ChopItTerrainAnalysis.h"

namespace ChopItTerrain
{
	double Fractal(double X, double Y, uint32 Seed, int32 Octaves, double Persistence, double Lacunarity)
	{
		double Sum = 0.0;
		double Weight = 0.0;
		double Amplitude = 1.0;
		for (int32 Octave = 0; Octave < Octaves; ++Octave)
		{
			Sum += Noise(X, Y, Mix(Seed + uint32(Octave))) * Amplitude;
			Weight += Amplitude;
			Amplitude *= Persistence;
			X *= Lacunarity;
			Y *= Lacunarity;
		}
		return Sum / Weight;
	}

	double Fractal(double X, double Y, uint32 Seed, const FChopItTerrainSettings& Settings)
	{
		return Fractal(X, Y, Seed, Settings.Octaves, Settings.Persistence, Settings.Lacunarity);
	}

	double Relief(double X, double Y, double EdgeDistance, const FChopItTerrainSettings& Settings)
	{
		const uint32 Seed = uint32(Settings.Seed);
		if (Settings.DomainWarpStrength > 0.0f)
		{
			const double Frequency = Settings.DomainWarpFrequency;
			const double WarpX = Noise(X * Frequency + 17.31, Y * Frequency - 22.71, Mix(Seed ^ 0xa13676f1u));
			const double WarpY = Noise(X * Frequency - 47.83, Y * Frequency + 8.19, Mix(Seed ^ 0x83b5a4ddu));
			X += Settings.DomainWarpStrength * WarpX;
			Y += Settings.DomainWarpStrength * WarpY;
		}
		// Fixed noninteger offsets avoid forcing all noise fields to zero at the terrain center.
		const double N = Fractal(X + 13.37, Y - 7.91, Seed, Settings);
		const double Hill = FMath::Max(0.0, N);
		const double Valley = FMath::Max(0.0, -N);
		const double Profile = Settings.HillStrength * FMath::Pow(Hill, Settings.HillShape) * 4.0
			- Settings.ValleyStrength * FMath::Pow(Valley, Settings.ValleyShape) * 4.0;
		const double PlainNoise = Noise(X * 0.55 + 31.17, Y * 0.55 - 19.83, Mix(Seed ^ 0x43ac891du));
		const double PlainMask = Settings.PlainsCoverage <= 0.0f ? 0.0 : Settings.PlainsCoverage >= 1.0f ? 1.0
			: Fade(FMath::Clamp((PlainNoise + 0.25 + (Settings.PlainsCoverage - 0.5) * 1.0) / 0.5, 0.0, 1.0));
		const double Roughness = Settings.RoughnessFrequency;
		const double Detail = Fractal(X * 8.0 * Roughness + 5.23, Y * 8.0 * Roughness + 41.73, Mix(Seed ^ 0xb794f126u), Settings);
		double Value = Profile * (1.0 - Settings.PlainsAmount * PlainMask) + Settings.SmallIrregularity * Detail;
		for (const FChopItTerrainNoiseLayer& Layer : Settings.NoiseLayers)
		{
			if (Layer.bEnabled && Layer.Strength > 0.0f)
			{
				const uint32 LayerSeed = Mix(Seed ^ uint32(Layer.SeedOffset) ^ 0x9ba4e103u);
				Value += Layer.Strength * Fractal(X * Layer.Frequency + 6.37, Y * Layer.Frequency - 12.79,
					LayerSeed, Layer.Octaves, Layer.Persistence, Layer.Lacunarity);
			}
		}
		if (Settings.DepressionStrength > 0.0f)
		{
			const double Depression = FMath::Max(0.0, Noise(X * Settings.DepressionFrequency + 61.7,
				Y * Settings.DepressionFrequency - 33.9, Mix(Seed ^ 0xd9286bafu)));
			Value -= Settings.DepressionStrength * Depression * Depression * 4.0;
		}
		if (Settings.EdgeFalloff > 0.0f)
		{
			const double T = FMath::Clamp((EdgeDistance - Settings.EdgeFalloffStart) / (1.0 - Settings.EdgeFalloffStart), 0.0, 1.0);
			Value *= 1.0 - Settings.EdgeFalloff * Fade(T);
		}
		return Value;
	}

	void RangeAndMean(const TArray<float>& Values, double& OutMin, double& OutMax, double& OutMean)
	{
		OutMin = TNumericLimits<double>::Max();
		OutMax = TNumericLimits<double>::Lowest();
		double Sum = 0.0;
		for (const float Value : Values)
		{
			Sum += Value;
			OutMin = FMath::Min(OutMin, double(Value));
			OutMax = FMath::Max(OutMax, double(Value));
		}
		OutMean = Sum / Values.Num();
	}

	void ApplySlopeBias(TArray<float>& Values, int32 Width, int32 Height, double Bias)
	{
		if (Bias == 0.0) return;
		TArray<float> Input = Values;
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				const int32 Index = X + Y * Width;
				double NeighborSum = 0.0;
				int32 Count = 0;
				if (X > 0) { NeighborSum += Input[Index - 1]; ++Count; }
				if (X + 1 < Width) { NeighborSum += Input[Index + 1]; ++Count; }
				if (Y > 0) { NeighborSum += Input[Index - Width]; ++Count; }
				if (Y + 1 < Height) { NeighborSum += Input[Index + Width]; ++Count; }
				Values[Index] = float(double(Input[Index]) + Bias * 0.2 * (double(Input[Index]) - NeighborSum / Count));
			}
		}
	}

	void ApplyErosion(TArray<float>& Values, int32 Width, int32 Height, double Strength, int32 Iterations)
	{
		if (Strength == 0.0) return;
		double Min, Max, Mean;
		RangeAndMean(Values, Min, Max, Mean);
		const double Talus = (Max - Min) * 0.02;
		for (int32 Iteration = 0; Iteration < Iterations; ++Iteration)
		{
			TArray<float> Delta;
			Delta.Init(0.0f, Values.Num());
			for (int32 Y = 0; Y < Height; ++Y)
			{
				for (int32 X = 0; X < Width; ++X)
				{
					const int32 A = X + Y * Width;
					for (int32 B : { X + 1 < Width ? A + 1 : -1, Y + 1 < Height ? A + Width : -1 })
					{
						if (B < 0) continue;
						const double Difference = double(Values[A]) - Values[B];
						const double Transfer = FMath::Max(0.0, FMath::Abs(Difference) - Talus) * 0.12 * Strength;
						if (Difference > 0.0) { Delta[A] -= float(Transfer); Delta[B] += float(Transfer); }
						else { Delta[A] += float(Transfer); Delta[B] -= float(Transfer); }
					}
				}
			}
			for (int32 Index = 0; Index < Values.Num(); ++Index) Values[Index] += Delta[Index];
		}
	}

	double CurveValue(double Input, const TArray<FVector2D>& Curve)
	{
		for (int32 Index = 1; Index < Curve.Num(); ++Index)
		{
			if (Input <= Curve[Index].X)
			{
				const FVector2D& A = Curve[Index - 1];
				const FVector2D& B = Curve[Index];
				return FMath::Lerp(A.Y, B.Y, (Input - A.X) / (B.X - A.X));
			}
		}
		return 1.0;
	}

	void FlattenSpawnArea(TArray<float>& Heights, const FChopItTerrainSettings& Settings)
	{
		if (Settings.SpawnFlatRadius <= 0.0f) return;
		const double StepX = Settings.Width / (Settings.SamplesX - 1);
		const double StepY = Settings.Length / (Settings.SamplesY - 1);
		const double GridX = FMath::Clamp((Settings.SpawnFlatCenter.X / Settings.Width + 0.5) * (Settings.SamplesX - 1), 0.0, double(Settings.SamplesX - 1));
		const double GridY = FMath::Clamp((Settings.SpawnFlatCenter.Y / Settings.Length + 0.5) * (Settings.SamplesY - 1), 0.0, double(Settings.SamplesY - 1));
		const int32 X0 = FMath::Min(FMath::FloorToInt(GridX), Settings.SamplesX - 2);
		const int32 Y0 = FMath::Min(FMath::FloorToInt(GridY), Settings.SamplesY - 2);
		const int32 I = X0 + Y0 * Settings.SamplesX;
		const double A = FMath::Lerp(double(Heights[I]), double(Heights[I + 1]), GridX - X0);
		const double B = FMath::Lerp(double(Heights[I + Settings.SamplesX]), double(Heights[I + Settings.SamplesX + 1]), GridX - X0);
		const double FlatHeight = FMath::Lerp(A, B, GridY - Y0);
		// Pad by one grid diagonal: every quad touching the requested radius has flat corners.
		const double InnerRadius = Settings.SpawnFlatRadius + FMath::Sqrt(StepX * StepX + StepY * StepY);
		for (int32 Y = 0; Y < Settings.SamplesY; ++Y)
		{
			const double WorldY = (double(Y) / (Settings.SamplesY - 1) - 0.5) * Settings.Length;
			for (int32 X = 0; X < Settings.SamplesX; ++X)
			{
				const double WorldX = (double(X) / (Settings.SamplesX - 1) - 0.5) * Settings.Width;
				const double Distance = FVector2D::Distance(FVector2D(WorldX, WorldY), Settings.SpawnFlatCenter);
				if (Distance <= InnerRadius) Heights[X + Y * Settings.SamplesX] = float(FlatHeight);
				else if (Settings.SpawnFlatBlendWidth > 0.0f && Distance < InnerRadius + Settings.SpawnFlatBlendWidth)
				{
					const double T = (Distance - InnerRadius) / Settings.SpawnFlatBlendWidth;
					const double Smooth = T * T * (3.0 - 2.0 * T);
					float& Height = Heights[X + Y * Settings.SamplesX];
					Height = float(FMath::Lerp(FlatHeight, double(Height), Smooth));
				}
			}
		}
	}
}

bool FChopItTerrainGenerator::Generate(const FChopItTerrainSettings& Settings, FChopItHeightField& OutField, FString& OutError)
{
	if (!Settings.Validate(OutError)) return false;

	FChopItHeightField Result;
	Result.SamplesX = Settings.SamplesX;
	Result.SamplesY = Settings.SamplesY;
	Result.Size = FVector2D(Settings.Width, Settings.Length);
	Result.Heights.SetNumUninitialized(Settings.SamplesX * Settings.SamplesY);

	for (int32 Y = 0; Y < Settings.SamplesY; ++Y)
	{
		const double UnitY = double(Y) / (Settings.SamplesY - 1) - 0.5;
		const double NY = UnitY * Settings.Length / Settings.ReliefScale;
		for (int32 X = 0; X < Settings.SamplesX; ++X)
		{
			const double UnitX = double(X) / (Settings.SamplesX - 1) - 0.5;
			const double NX = UnitX * Settings.Width / Settings.ReliefScale;
			const double EdgeDistance = 2.0 * FMath::Max(FMath::Abs(UnitX), FMath::Abs(UnitY));
			const float Value = float(ChopItTerrain::Relief(NX, NY, EdgeDistance, Settings));
			Result.Heights[X + Y * Settings.SamplesX] = Value;
		}
	}
	ChopItTerrain::ApplySlopeBias(Result.Heights, Settings.SamplesX, Settings.SamplesY, Settings.SlopeBias);
	ChopItTerrain::ApplyErosion(Result.Heights, Settings.SamplesX, Settings.SamplesY, Settings.ErosionStrength, Settings.ErosionIterations);
	double RawMin, RawMax, RawMean;
	ChopItTerrain::RangeAndMean(Result.Heights, RawMin, RawMax, RawMean);
	if (RawMax > RawMin && (!Settings.HeightDistributionCurve.IsEmpty() || Settings.TerraceStrength > 0.0f))
	{
		for (float& Height : Result.Heights)
		{
			double Normalized = FMath::Clamp((double(Height) - RawMin) / (RawMax - RawMin), 0.0, 1.0);
			if (!Settings.HeightDistributionCurve.IsEmpty()) Normalized = ChopItTerrain::CurveValue(Normalized, Settings.HeightDistributionCurve);
			if (Settings.TerraceStrength > 0.0f)
			{
				const double Steps = Settings.TerraceCount - 1;
				const double Terrace = FMath::FloorToInt(Normalized * Steps + 0.5) / Steps;
				Normalized = FMath::Lerp(Normalized, Terrace, double(Settings.TerraceStrength));
			}
			Height = float(RawMin + Normalized * (RawMax - RawMin));
		}
		ChopItTerrain::RangeAndMean(Result.Heights, RawMin, RawMax, RawMean);
	}

	ChopItTerrain::FlattenSpawnArea(Result.Heights, Settings);
	ChopItTerrain::RangeAndMean(Result.Heights, RawMin, RawMax, RawMean);
	// One global affine fit preserves the shape and target mean, avoiding clipped plateaus.
	double Scale = Settings.HeightVariation;
	if (RawMean > RawMin) Scale = FMath::Min(Scale, (double(Settings.TargetMeanHeight) - Settings.MinHeight) / (RawMean - RawMin));
	if (RawMax > RawMean) Scale = FMath::Min(Scale, (double(Settings.MaxHeight) - Settings.TargetMeanHeight) / (RawMax - RawMean));
	double Sum = 0.0;
	Result.MinHeight = TNumericLimits<float>::Max();
	Result.MaxHeight = TNumericLimits<float>::Lowest();
	for (float& Height : Result.Heights)
	{
		Height = float(FMath::Clamp(double(Settings.TargetMeanHeight) + (double(Height) - RawMean) * Scale, double(Settings.MinHeight), double(Settings.MaxHeight)));
		Sum += Height;
		Result.MinHeight = FMath::Min(Result.MinHeight, Height);
		Result.MaxHeight = FMath::Max(Result.MaxHeight, Height);
	}
	Result.MeanHeight = Sum / Result.Heights.Num();
	// Every grid cell covers the same XY area. Its bilinear center estimates submerged surface coverage.
	TArray<float> CellHeights;
	CellHeights.Reserve((Settings.SamplesX - 1) * (Settings.SamplesY - 1));
	for (int32 Y = 0; Y + 1 < Settings.SamplesY; ++Y)
	{
		for (int32 X = 0; X + 1 < Settings.SamplesX; ++X)
		{
			const int32 Index = X + Y * Settings.SamplesX;
			CellHeights.Add(float((double(Result.Heights[Index]) + Result.Heights[Index + 1]
				+ Result.Heights[Index + Settings.SamplesX] + Result.Heights[Index + Settings.SamplesX + 1]) * 0.25));
		}
	}
	Result.WaterLevel = Settings.WaterLevel;
	if (Settings.bAutoWaterLevel)
	{
		TArray<float>& Sorted = CellHeights;
		Sorted.Sort();
		const int32 SubmergedCount = FMath::Clamp(FMath::RoundToInt(Settings.TargetSubmergedFraction * Sorted.Num()), 0, Sorted.Num());
		if (SubmergedCount == 0) Result.WaterLevel = Sorted[0] - 1.0f;
		else if (SubmergedCount == Sorted.Num()) Result.WaterLevel = Sorted.Last();
		else Result.WaterLevel = float((double(Sorted[SubmergedCount - 1]) + Sorted[SubmergedCount]) * 0.5);
	}
	int32 ActualSubmerged = 0;
	for (const float Height : CellHeights) ActualSubmerged += Height <= Result.WaterLevel ? 1 : 0;
	Result.SubmergedFraction = double(ActualSubmerged) / CellHeights.Num();
	if (!FChopItTerrainAnalysis::Build(Result.Heights, FIntPoint(Result.SamplesX, Result.SamplesY), Result.Size,
		Result.WaterLevel, Settings.Seed, Settings.Regions, Result.Maps, OutError)) return false;
	OutField = MoveTemp(Result);
	return true;
}
