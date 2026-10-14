#include "Terrain/ChopItTerrainGenerator.h"
#include "Misc/AutomationTest.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

namespace ChopItTerrainTests
{
	FChopItTerrainSettings SmallSettings()
	{
		FChopItTerrainSettings Settings;
		Settings.SamplesX = 65;
		Settings.SamplesY = 49;
		return Settings;
	}

	double RMS(const FChopItHeightField& Field)
	{
		double Sum = 0.0;
		for (float Height : Field.GetHeights()) Sum += FMath::Square(double(Height) - Field.GetMeanHeight());
		return FMath::Sqrt(Sum / Field.GetHeights().Num());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainDeterminismTest, "ChopIt.Terrain.Core.Determinism", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainDeterminismTest::RunTest(const FString& Parameters)
{
	FChopItTerrainSettings Settings = ChopItTerrainTests::SmallSettings();
	FChopItHeightField First, Other, Repeated;
	FString Error;
	for (int32 Seed : { 0, 1337, -1, TNumericLimits<int32>::Lowest(), TNumericLimits<int32>::Max() })
	{
		Settings.Seed = Seed;
		if (!TestTrue(TEXT("Generate first field"), FChopItTerrainGenerator::Generate(Settings, First, Error))) return false;
		FChopItTerrainSettings OtherSettings = Settings;
		OtherSettings.Seed = Seed == 0 ? 1 : 0;
		if (!TestTrue(TEXT("Generate unrelated field"), FChopItTerrainGenerator::Generate(OtherSettings, Other, Error))) return false;
		if (!TestTrue(TEXT("Regenerate original seed"), FChopItTerrainGenerator::Generate(Settings, Repeated, Error))) return false;
		TestTrue(TEXT("Same seed is exactly repeatable after unrelated generation"), First.GetHeights() == Repeated.GetHeights());
		TestFalse(TEXT("Different seeds change terrain"), First.GetHeights() == Other.GetHeights());
		TestTrue(TEXT("Diagnostic is cleared on success"), Error.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainHeightContractTest, "ChopIt.Terrain.Core.HeightContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainHeightContractTest::RunTest(const FString& Parameters)
{
	FChopItTerrainSettings Settings = ChopItTerrainTests::SmallSettings();
	FChopItHeightField Field;
	FString Error;
	for (int32 Case = 0; Case < 6; ++Case)
	{
		Settings = ChopItTerrainTests::SmallSettings();
		if (Case == 1) { Settings.MinHeight = -1700.0f; Settings.MaxHeight = -300.0f; Settings.TargetMeanHeight = -600.0f; }
		if (Case == 2) { Settings.MinHeight = 17.0f; Settings.MaxHeight = 18.0f; Settings.TargetMeanHeight = 17.1f; Settings.HeightVariation = 100000.0f; }
		if (Case == 3) { Settings.HeightVariation = 0.0f; }
		if (Case == 4) { Settings.MinHeight = Settings.MaxHeight = Settings.TargetMeanHeight = 42.0f; }
		if (Case == 5) { Settings.TargetMeanHeight = Settings.MinHeight; }
		if (!TestTrue(TEXT("Generate valid bounded field"), FChopItTerrainGenerator::Generate(Settings, Field, Error))) return false;
		TestTrue(TEXT("Mean matches target within 0.001 cm"), FMath::Abs(Field.GetMeanHeight() - Settings.TargetMeanHeight) < 0.001);
		bool bWithinBounds = true;
		for (float Height : Field.GetHeights()) bWithinBounds &= FMath::IsFinite(Height) && Height >= Settings.MinHeight && Height <= Settings.MaxHeight;
		TestTrue(TEXT("Every sample is finite and bounded"), bWithinBounds);
		if (Case >= 3) TestEqual(TEXT("Degenerate range or amplitude is flat"), Field.GetMinHeight(), Field.GetMaxHeight());
		else TestTrue(TEXT("Nondegenerate settings retain variation"), Field.GetMaxHeight() > Field.GetMinHeight());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainParametersTest, "ChopIt.Terrain.Core.ParameterEffects", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainParametersTest::RunTest(const FString& Parameters)
{
	FChopItTerrainSettings Base = ChopItTerrainTests::SmallSettings();
	Base.MinHeight = -10000.0f;
	Base.MaxHeight = 10000.0f;
	Base.HeightVariation = 100.0f;
	FChopItHeightField Reference, Changed;
	FString Error;
	if (!TestTrue(TEXT("Generate reference"), FChopItTerrainGenerator::Generate(Base, Reference, Error))) return false;
	const TCHAR* Names[] = { TEXT("Width"), TEXT("Length"), TEXT("ReliefScale"), TEXT("PlainsAmount"), TEXT("HillStrength"), TEXT("ValleyStrength"), TEXT("SmallIrregularity"), TEXT("Octaves"), TEXT("Persistence"), TEXT("Lacunarity") };
	for (int32 Case = 0; Case < UE_ARRAY_COUNT(Names); ++Case)
	{
		FChopItTerrainSettings Settings = Base;
		switch (Case)
		{
		case 0: Settings.Width *= 1.5; break;
		case 1: Settings.Length *= 1.5; break;
		case 2: Settings.ReliefScale *= 2.0; break;
		case 3: Settings.PlainsAmount = 0.0f; break;
		case 4: Settings.HillStrength = 0.0f; break;
		case 5: Settings.ValleyStrength = 0.0f; break;
		case 6: Settings.SmallIrregularity = 0.0f; break;
		case 7: Settings.Octaves = 1; break;
		case 8: Settings.Persistence = 0.8f; break;
		case 9: Settings.Lacunarity = 3.0f; break;
		}
		if (!TestTrue(Names[Case], FChopItTerrainGenerator::Generate(Settings, Changed, Error))) return false;
		TestFalse(FString::Printf(TEXT("%s changes the field"), Names[Case]), Reference.GetHeights() == Changed.GetHeights());
	}
	FChopItTerrainSettings Settings = Base;
	Settings.HeightVariation *= 2.0f;
	if (!FChopItTerrainGenerator::Generate(Settings, Changed, Error)) return false;
	TestTrue(TEXT("Doubling unclipped amplitude doubles RMS relief"), FMath::Abs(ChopItTerrainTests::RMS(Changed) / ChopItTerrainTests::RMS(Reference) - 2.0) < 0.0001);
	Settings = Base;
	Settings.TargetMeanHeight += 200.0f;
	if (!FChopItTerrainGenerator::Generate(Settings, Changed, Error)) return false;
	bool bTranslated = true;
	for (int32 Index = 0; Index < Reference.GetHeights().Num(); ++Index) bTranslated &= FMath::Abs(Changed.GetHeights()[Index] - Reference.GetHeights()[Index] - 200.0f) < 0.001f;
	TestTrue(TEXT("Mean translates terrain without changing its shape"), bTranslated);
	Settings = Base;
	Settings.SmallIrregularity = 0.0f;
	Settings.PlainsAmount = 0.0f;
	if (!FChopItTerrainGenerator::Generate(Settings, Reference, Error)) return false;
	Settings.PlainsAmount = 1.0f;
	if (!FChopItTerrainGenerator::Generate(Settings, Changed, Error)) return false;
	TestTrue(TEXT("Plains reduce the broad relief amplitude"), ChopItTerrainTests::RMS(Changed) < ChopItTerrainTests::RMS(Reference));
	Settings.HillStrength = Settings.ValleyStrength = 0.0f;
	if (!FChopItTerrainGenerator::Generate(Settings, Changed, Error)) return false;
	TestEqual(TEXT("Disabling all relief yields a flat heightmap"), Changed.GetMinHeight(), Changed.GetMaxHeight());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainSamplingTest, "ChopIt.Terrain.Core.Sampling", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainSamplingTest::RunTest(const FString& Parameters)
{
	FChopItTerrainSettings Settings = ChopItTerrainTests::SmallSettings();
	Settings.Length = 12000.0;
	FChopItHeightField Field;
	FString Error;
	float Height = 99.0f;
	TestFalse(TEXT("Empty heightmap cannot be sampled"), Field.SampleLocalHeight(FVector2D::ZeroVector, Height));
	if (!TestTrue(TEXT("Generate rectangular field"), FChopItTerrainGenerator::Generate(Settings, Field, Error))) return false;
	TestEqual(TEXT("Exact sample count"), Field.GetHeights().Num(), Settings.SamplesX * Settings.SamplesY);
	TestEqual(TEXT("X metadata"), Field.GetSamplesX(), Settings.SamplesX);
	TestEqual(TEXT("Y metadata"), Field.GetSamplesY(), Settings.SamplesY);
	bool bGridMatches = true;
	for (int32 Y = 0; Y < Settings.SamplesY; ++Y)
	{
		for (int32 X = 0; X < Settings.SamplesX; ++X)
		{
			const FVector2D XY((double(X) / (Settings.SamplesX - 1) - 0.5) * Settings.Width, (double(Y) / (Settings.SamplesY - 1) - 0.5) * Settings.Length);
			bGridMatches &= Field.SampleLocalHeight(XY, Height) && FMath::Abs(Height - Field.GetHeights()[X + Y * Settings.SamplesX]) < 0.001f;
		}
	}
	TestTrue(TEXT("Every grid sample including all four borders round-trips"), bGridMatches);
	const FVector2D CellCenter((10.5 / (Settings.SamplesX - 1) - 0.5) * Settings.Width, (8.5 / (Settings.SamplesY - 1) - 0.5) * Settings.Length);
	TestTrue(TEXT("Sample cell center"), Field.SampleLocalHeight(CellCenter, Height));
	const TArray<float>& H = Field.GetHeights();
	const float Expected = (H[10 + 8 * Settings.SamplesX] + H[11 + 8 * Settings.SamplesX] + H[10 + 9 * Settings.SamplesX] + H[11 + 9 * Settings.SamplesX]) * 0.25f;
	TestTrue(TEXT("Cell center is bilinear average"), FMath::Abs(Height - Expected) < 0.001f);
	TestFalse(TEXT("Outside position fails"), Field.SampleLocalHeight(FVector2D(Settings.Width, 0.0), Height));
	TestFalse(TEXT("Nonfinite position fails"), Field.SampleLocalHeight(FVector2D(std::numeric_limits<double>::quiet_NaN(), 0.0), Height));
	TestEqual(TEXT("Failed sample resets output"), Height, 0.0f);
	Settings.SamplesX = Settings.SamplesY = 2;
	TestTrue(TEXT("Smallest valid grid generates"), FChopItTerrainGenerator::Generate(Settings, Field, Error));
	TestTrue(TEXT("Smallest grid interpolates"), Field.SampleLocalHeight(FVector2D::ZeroVector, Height));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainValidationTest, "ChopIt.Terrain.Core.Validation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainValidationTest::RunTest(const FString& Parameters)
{
	FChopItTerrainSettings Base = ChopItTerrainTests::SmallSettings();
	FChopItHeightField Field;
	FString Error;
	if (!TestTrue(TEXT("Generate valid field before failures"), FChopItTerrainGenerator::Generate(Base, Field, Error))) return false;
	const TArray<float> Original = Field.GetHeights();
	for (int32 Case = 0; Case < 16; ++Case)
	{
		FChopItTerrainSettings Settings = Base;
		switch (Case)
		{
		case 0: Settings.SamplesX = 1; break;
		case 1: Settings.SamplesY = TNumericLimits<int32>::Max(); break;
		case 2: Settings.Width = 0.0; break;
		case 3: Settings.Length = std::numeric_limits<double>::infinity(); break;
		case 4: Settings.ReliefScale = std::numeric_limits<double>::quiet_NaN(); break;
		case 5: Settings.MinHeight = Settings.MaxHeight + 1.0f; break;
		case 6: Settings.TargetMeanHeight = Settings.MinHeight - 1.0f; break;
		case 7: Settings.MaxHeight = std::numeric_limits<float>::infinity(); break;
		case 8: Settings.HeightVariation = -1.0f; break;
		case 9: Settings.PlainsAmount = 2.0f; break;
		case 10: Settings.HillStrength = -1.0f; break;
		case 11: Settings.ValleyStrength = 3.0f; break;
		case 12: Settings.SmallIrregularity = std::numeric_limits<float>::quiet_NaN(); break;
		case 13: Settings.Octaves = 9; break;
		case 14: Settings.Persistence = -0.1f; break;
		case 15: Settings.Lacunarity = 0.0f; break;
		}
		TestFalse(TEXT("Reject invalid settings"), FChopItTerrainGenerator::Generate(Settings, Field, Error));
		TestFalse(TEXT("Provide diagnostic"), Error.IsEmpty());
		TestTrue(TEXT("Failure preserves previous valid output"), Field.GetHeights() == Original);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainAdvancedControlsTest, "ChopIt.Terrain.Core.AdvancedControls", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainAdvancedControlsTest::RunTest(const FString& Parameters)
{
	FChopItTerrainSettings Base = ChopItTerrainTests::SmallSettings();
	Base.MinHeight = -10000.0f;
	Base.MaxHeight = 10000.0f;
	Base.HeightVariation = 500.0f;
	FChopItHeightField Reference, Changed, Repeated;
	FString Error;
	if (!TestTrue(TEXT("Generate advanced reference"), FChopItTerrainGenerator::Generate(Base, Reference, Error))) return false;
	const TCHAR* Names[] = { TEXT("HillShape"), TEXT("ValleyShape"), TEXT("PlainsCoverage"), TEXT("SlopeBias"),
		TEXT("RoughnessFrequency"), TEXT("DomainWarpStrength"), TEXT("EdgeFalloff"), TEXT("HeightDistributionCurve"),
		TEXT("DepressionStrength"), TEXT("TerraceStrength"), TEXT("ErosionStrength"), TEXT("NoiseLayers") };
	for (int32 Case = 0; Case < UE_ARRAY_COUNT(Names); ++Case)
	{
		FChopItTerrainSettings Settings = Base;
		switch (Case)
		{
		case 0: Settings.HillShape = 0.8f; break;
		case 1: Settings.ValleyShape = 3.5f; break;
		case 2: Settings.PlainsCoverage = 0.9f; break;
		case 3: Settings.SlopeBias = -1.0f; break;
		case 4: Settings.RoughnessFrequency = 4.0f; break;
		case 5: Settings.DomainWarpStrength = 1.2f; break;
		case 6: Settings.EdgeFalloff = 1.0f; break;
		case 7: Settings.HeightDistributionCurve = { FVector2D(0.0, 0.0), FVector2D(0.5, 0.2), FVector2D(1.0, 1.0) }; break;
		case 8: Settings.DepressionStrength = 1.0f; break;
		case 9: Settings.TerraceStrength = 1.0f; break;
		case 10: Settings.ErosionStrength = 1.0f; Settings.ErosionIterations = 4; break;
		case 11: Settings.NoiseLayers.AddDefaulted(); break;
		}
		if (!TestTrue(Names[Case], FChopItTerrainGenerator::Generate(Settings, Changed, Error))) return false;
		TestFalse(FString::Printf(TEXT("%s changes samples"), Names[Case]), Reference.GetHeights() == Changed.GetHeights());
		TestTrue(FString::Printf(TEXT("%s preserves the target mean"), Names[Case]), FMath::Abs(Changed.GetMeanHeight() - Settings.TargetMeanHeight) < 0.001);
		if (!TestTrue(TEXT("Advanced settings regenerate"), FChopItTerrainGenerator::Generate(Settings, Repeated, Error))) return false;
		TestTrue(FString::Printf(TEXT("%s is deterministic"), Names[Case]), Changed.GetHeights() == Repeated.GetHeights());
	}
	FChopItTerrainSettings Settings = Base;
	Settings.PlainsAmount = 1.0f;
	Settings.SmallIrregularity = 0.0f;
	Settings.PlainsCoverage = 0.0f;
	if (!FChopItTerrainGenerator::Generate(Settings, Reference, Error)) return false;
	Settings.PlainsCoverage = 1.0f;
	if (!FChopItTerrainGenerator::Generate(Settings, Changed, Error)) return false;
	TestEqual(TEXT("Full plain coverage and flattening removes main relief"), Changed.GetMinHeight(), Changed.GetMaxHeight());
	TestTrue(TEXT("Zero plain coverage retains relief"), Reference.GetMaxHeight() > Reference.GetMinHeight());
	const auto GradientEnergy = [](const FChopItHeightField& Field)
	{
		double Energy = 0.0;
		for (int32 Y = 0; Y < Field.GetSamplesY(); ++Y)
			for (int32 X = 0; X + 1 < Field.GetSamplesX(); ++X)
				Energy += FMath::Square(double(Field.GetHeights()[X + 1 + Y * Field.GetSamplesX()]) - Field.GetHeights()[X + Y * Field.GetSamplesX()]);
		return Energy;
	};
	Settings = Base;
	Settings.SlopeBias = -1.0f;
	if (!FChopItTerrainGenerator::Generate(Settings, Reference, Error)) return false;
	Settings.SlopeBias = 1.0f;
	if (!FChopItTerrainGenerator::Generate(Settings, Changed, Error)) return false;
	TestTrue(TEXT("Positive slope bias has more local gradient energy than negative bias"), GradientEnergy(Changed) > GradientEnergy(Reference));
	Settings = Base;
	Settings.EdgeFalloff = 1.0f;
	if (!FChopItTerrainGenerator::Generate(Settings, Changed, Error)) return false;
	bool bUniformBoundary = true;
	for (int32 Y = 0; Y < Changed.GetSamplesY(); ++Y)
		for (int32 X = 0; X < Changed.GetSamplesX(); ++X)
			if (X == 0 || Y == 0 || X + 1 == Changed.GetSamplesX() || Y + 1 == Changed.GetSamplesY())
				bUniformBoundary &= Changed.GetHeights()[X + Y * Changed.GetSamplesX()] == Changed.GetHeights()[0];
	TestTrue(TEXT("Full edge falloff yields one continuous border elevation"), bUniformBoundary);
	Settings = Base;
	Settings.TerraceStrength = 1.0f;
	Settings.TerraceCount = 5;
	if (!FChopItTerrainGenerator::Generate(Settings, Changed, Error)) return false;
	TSet<float> Levels;
	for (float Height : Changed.GetHeights()) Levels.Add(Height);
	TestTrue(TEXT("Full terraces have at most the requested elevation levels"), Levels.Num() <= Settings.TerraceCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainWaterTest, "ChopIt.Terrain.Core.WaterCoverage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainWaterTest::RunTest(const FString& Parameters)
{
	FChopItTerrainSettings Settings = ChopItTerrainTests::SmallSettings();
	FChopItHeightField Field;
	FString Error;
	Settings.WaterLevel = -10000.0f;
	if (!TestTrue(TEXT("Manual water below terrain"), FChopItTerrainGenerator::Generate(Settings, Field, Error))) return false;
	TestEqual(TEXT("Manual water level is retained"), Field.GetWaterLevel(), Settings.WaterLevel);
	TestEqual(TEXT("No submerged samples"), Field.GetSubmergedFraction(), 0.0);
	Settings.WaterLevel = 10000.0f;
	if (!FChopItTerrainGenerator::Generate(Settings, Field, Error)) return false;
	TestEqual(TEXT("All samples submerged"), Field.GetSubmergedFraction(), 1.0);
	Settings.bAutoWaterLevel = true;
	for (float Target : { 0.0f, 0.3f, 0.7f, 1.0f })
	{
		Settings.TargetSubmergedFraction = Target;
		if (!TestTrue(TEXT("Automatic water level generates"), FChopItTerrainGenerator::Generate(Settings, Field, Error))) return false;
		TestTrue(TEXT("Estimated area coverage matches target within one cell"), FMath::Abs(Field.GetSubmergedFraction() - Target) <= 1.0 / ((Field.GetSamplesX() - 1) * (Field.GetSamplesY() - 1)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainDependentControlsTest, "ChopIt.Terrain.Core.DependentControls", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainDependentControlsTest::RunTest(const FString& Parameters)
{
	FChopItTerrainSettings Settings = ChopItTerrainTests::SmallSettings();
	Settings.MinHeight = -10000.0f;
	Settings.MaxHeight = 10000.0f;
	Settings.HeightVariation = 500.0f;
	FChopItHeightField Reference, Changed;
	FString Error;
	for (int32 Case = 0; Case < 11; ++Case)
	{
		FChopItTerrainSettings Base = Settings;
		FChopItTerrainSettings Variant = Settings;
		switch (Case)
		{
		case 0: Base.DomainWarpStrength = Variant.DomainWarpStrength = 1.0f; Variant.DomainWarpFrequency = 3.0f; break;
		case 1: Base.EdgeFalloff = Variant.EdgeFalloff = 1.0f; Variant.EdgeFalloffStart = 0.2f; break;
		case 2: Base.DepressionStrength = Variant.DepressionStrength = 1.0f; Variant.DepressionFrequency = 4.0f; break;
		case 3: Base.TerraceStrength = Variant.TerraceStrength = 1.0f; Variant.TerraceCount = 4; break;
		case 4: Base.ErosionStrength = Variant.ErosionStrength = 1.0f; Variant.ErosionIterations = 7; break;
		case 5: Base.NoiseLayers.AddDefaulted(); Variant.NoiseLayers = Base.NoiseLayers; Variant.NoiseLayers[0].bEnabled = false; break;
		case 6: Base.NoiseLayers.AddDefaulted(); Variant.NoiseLayers = Base.NoiseLayers; Variant.NoiseLayers[0].Strength = 0.8f; break;
		case 7: Base.NoiseLayers.AddDefaulted(); Variant.NoiseLayers = Base.NoiseLayers; Variant.NoiseLayers[0].Frequency = 5.0f; break;
		case 8: Base.NoiseLayers.AddDefaulted(); Variant.NoiseLayers = Base.NoiseLayers; Variant.NoiseLayers[0].Octaves = 1; break;
		case 9: Base.NoiseLayers.AddDefaulted(); Variant.NoiseLayers = Base.NoiseLayers; Variant.NoiseLayers[0].Persistence = 0.9f; break;
		case 10: Base.NoiseLayers.AddDefaulted(); Variant.NoiseLayers = Base.NoiseLayers; Variant.NoiseLayers[0].Lacunarity = 3.0f; break;
		}
		if (!TestTrue(TEXT("Generate enabled dependent control reference"), FChopItTerrainGenerator::Generate(Base, Reference, Error))) return false;
		if (!TestTrue(TEXT("Generate dependent control variant"), FChopItTerrainGenerator::Generate(Variant, Changed, Error))) return false;
		TestFalse(FString::Printf(TEXT("Dependent control %d changes terrain"), Case), Reference.GetHeights() == Changed.GetHeights());
	}
	FChopItTerrainSettings First = Settings;
	First.NoiseLayers.AddDefaulted();
	FChopItTerrainSettings Second = First;
	Second.NoiseLayers[0].SeedOffset = 2;
	if (!FChopItTerrainGenerator::Generate(First, Reference, Error) || !FChopItTerrainGenerator::Generate(Second, Changed, Error)) return false;
	TestFalse(TEXT("Independent layer seed changes terrain"), Reference.GetHeights() == Changed.GetHeights());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainAdvancedValidationTest, "ChopIt.Terrain.Core.AdvancedValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainAdvancedValidationTest::RunTest(const FString& Parameters)
{
	const FChopItTerrainSettings Base = ChopItTerrainTests::SmallSettings();
	FChopItHeightField Field;
	FString Error;
	if (!FChopItTerrainGenerator::Generate(Base, Field, Error)) return false;
	const TArray<float> Original = Field.GetHeights();
	for (int32 Case = 0; Case < 10; ++Case)
	{
		FChopItTerrainSettings Settings = Base;
		switch (Case)
		{
		case 0: Settings.HillShape = 0.0f; break;
		case 1: Settings.ValleyShape = 5.0f; break;
		case 2: Settings.PlainsCoverage = -0.1f; break;
		case 3: Settings.SlopeBias = 2.0f; break;
		case 4: Settings.DomainWarpFrequency = 0.0f; break;
		case 5: Settings.HeightDistributionCurve = { FVector2D(0.0, 0.0), FVector2D(0.6, 0.7), FVector2D(0.5, 1.0), FVector2D(1.0, 1.0) }; break;
		case 6: Settings.TerraceCount = 1; break;
		case 7: Settings.ErosionIterations = 9; break;
		case 8: Settings.TargetSubmergedFraction = 1.1f; break;
		case 9: Settings.NoiseLayers.AddDefaulted(); Settings.NoiseLayers[0].Octaves = 0; break;
		}
		TestFalse(TEXT("Reject invalid advanced settings"), FChopItTerrainGenerator::Generate(Settings, Field, Error));
		TestFalse(TEXT("Advanced validation has diagnostic"), Error.IsEmpty());
		TestTrue(TEXT("Invalid settings preserve previous output"), Field.GetHeights() == Original);
	}
	return true;
}

#endif
