#include "Terrain/ChopItTerrainAnalysis.h"
#include "Terrain/ChopItTerrainGenerator.h"
#include "Misc/AutomationTest.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
TArray<float> Surface(FIntPoint Samples, FVector2D Size, TFunctionRef<double(double, double)> Elevation)
{
	TArray<float> Heights;
	for (int32 Y = 0; Y < Samples.Y; ++Y)
		for (int32 X = 0; X < Samples.X; ++X)
			Heights.Add(float(Elevation((double(X) / (Samples.X - 1) - 0.5) * Size.X, (double(Y) / (Samples.Y - 1) - 0.5) * Size.Y)));
	return Heights;
}
double Average(const TArray<float>& Values)
{
	double Sum = 0;
	for (float Value : Values) Sum += Value;
	return Sum / Values.Num();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainAnalyticMapsTest, "ChopIt.Terrain.Maps.AnalyticSurfaces", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainAnalyticMapsTest::RunTest(const FString& Parameters)
{
	FChopItTerrainRegionSettings Settings;
	Settings.ClearingCoverage = 0;
	const FIntPoint Samples(11, 7);
	const FVector2D Size(2000, 1800);
	FChopItTerrainMaps Maps;
	FString Error;
	for (int32 Case = 0; Case < 4; ++Case)
	{
		const double A = Case == 0 ? 0.0 : Case == 1 ? 0.001 : Case == 2 ? -0.001 : 0.0;
		const double B = Case == 0 ? 0.0 : Case == 1 ? 0.002 : Case == 2 ? -0.002 : 0.0;
		const double PlaneX = Case == 3 ? 0.5 : 0.0, PlaneY = Case == 3 ? 0.25 : 0.0;
		const auto H = Surface(Samples, Size, [=](double X, double Y) { return 5000 + A * X * X + B * Y * Y + PlaneX * X + PlaneY * Y; });
		if (!TestTrue(TEXT("Analyze known surface"), FChopItTerrainAnalysis::Build(H, Samples, Size, 0, 1337, Settings, Maps, Error))) return false;
		bool bSlopeCorrect = true, bCurvatureCorrect = true;
		for (int32 Y = 0; Y < Samples.Y; ++Y)
			for (int32 X = 0; X < Samples.X; ++X)
			{
				const double PX = (double(X) / (Samples.X - 1) - 0.5) * Size.X;
				const double PY = (double(Y) / (Samples.Y - 1) - 0.5) * Size.Y;
				const double ExpectedSlope = FMath::RadiansToDegrees(FMath::Atan(FMath::Sqrt(FMath::Square(2 * A * PX + PlaneX) + FMath::Square(2 * B * PY + PlaneY))));
				bSlopeCorrect &= FMath::Abs(Maps.SlopeDegrees[X + Y * Samples.X] - ExpectedSlope) < 0.001;
				bCurvatureCorrect &= FMath::Abs(Maps.Curvature[X + Y * Samples.X] - 2 * (A + B)) < 0.000001;
			}
		TestTrue(TEXT("Slope matches analytic gradient including borders and unequal spacing"), bSlopeCorrect);
		TestTrue(TEXT("Curvature matches analytic Laplacian including borders"), bCurvatureCorrect);
		if (Case == 1) TestTrue(TEXT("Bowl center is a valley"), Maps.Regions[5 + 3 * Samples.X] == EChopItTerrainRegion::Valleys);
	}
	const auto Tiny = Surface(FIntPoint(2, 2), FVector2D(100, 200), [](double X, double Y) { return 5000 + 0.5 * X + 0.25 * Y; });
	TestTrue(TEXT("Smallest lattice analyzes"), FChopItTerrainAnalysis::Build(Tiny, FIntPoint(2, 2), FVector2D(100, 200), 0, 1, Settings, Maps, Error));
	for (float Curvature : Maps.Curvature) TestEqual(TEXT("Two-point axes have no inferred curvature"), Curvature, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainRegionSemanticsTest, "ChopIt.Terrain.Maps.RegionSemantics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainRegionSemanticsTest::RunTest(const FString& Parameters)
{
	FChopItTerrainRegionSettings Settings;
	Settings.ClearingCoverage = 0;
	const FIntPoint Samples(17, 9);
	const FVector2D Size(2000, 1000);
	FChopItTerrainMaps Maps;
	FString Error;
	for (int32 Case = 0; Case < 4; ++Case)
	{
		const double Angle = Case == 0 ? 0.0 : Case == 1 ? 10.0 : Case == 2 ? 60.0 : 5.0;
		const auto H = Surface(Samples, Size, [=](double X, double Y) { return 5000 + FMath::Tan(FMath::DegreesToRadians(Angle)) * X; });
		if (!FChopItTerrainAnalysis::Build(H, Samples, Size, 0, 3, Settings, Maps, Error)) return false;
		const EChopItTerrainRegion Expected[] = { EChopItTerrainRegion::FlatForest, EChopItTerrainRegion::RollingHills, EChopItTerrainRegion::SteepHills, EChopItTerrainRegion::Transitional };
		for (EChopItTerrainRegion Region : Maps.Regions) if (!TestTrue(TEXT("Plane classification matches its slope"), Region == Expected[Case])) return false;
		if (Case == 2) TestEqual(TEXT("Steep surface excludes walkability"), Average(Maps.Walkability), 0.0);
	}
	const auto Flat = Surface(Samples, Size, [](double X, double Y) { return 500.0; });
	Settings.ClearingCoverage = 1;
	if (!FChopItTerrainAnalysis::Build(Flat, Samples, Size, 0, 3, Settings, Maps, Error)) return false;
	TestEqual(TEXT("All suitable flat land can be reserved as clearing"), Average(Maps.Clearing), 1.0);
	TestEqual(TEXT("Clearings suppress vegetation potential"), Average(Maps.VegetationDensityPotential), 0.0);
	for (auto Region : Maps.Regions) TestTrue(TEXT("Full clearing classification"), Region == EChopItTerrainRegion::Clearings);
	if (!FChopItTerrainAnalysis::Build(Flat, Samples, Size, 600, 3, Settings, Maps, Error)) return false;
	TestEqual(TEXT("Submerged land is not walkable"), Average(Maps.Walkability), 0.0);
	TestEqual(TEXT("Submerged land is not a clearing"), Average(Maps.Clearing), 0.0);
	TestEqual(TEXT("Submerged land has no vegetation potential"), Average(Maps.VegetationDensityPotential), 0.0);
	TestEqual(TEXT("Submerged land has maximum moisture potential"), Average(Maps.MoisturePotential), 1.0);
	for (auto Region : Maps.Regions) TestTrue(TEXT("Submerged land is lowlands"), Region == EChopItTerrainRegion::Lowlands);
	if (!FChopItTerrainAnalysis::Build(Flat, Samples, Size, 499, 3, Settings, Maps, Error)) return false;
	TestTrue(TEXT("Moisture stays continuous across the water line"), 1.0 - Average(Maps.MoisturePotential) < 0.00001);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainMapAlignmentTest, "ChopIt.Terrain.Maps.AlignmentAndDeterminism", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainMapAlignmentTest::RunTest(const FString& Parameters)
{
	FChopItTerrainSettings Settings;
	Settings.SamplesX = 65; Settings.SamplesY = 49;
	Settings.Width = 20000; Settings.Length = 13000;
	FChopItHeightField Field, Repeat;
	FString Error;
	if (!FChopItTerrainGenerator::Generate(Settings, Field, Error) || !FChopItTerrainGenerator::Generate(Settings, Repeat, Error)) return false;
	const auto& Maps = Field.GetMaps();
	TestTrue(TEXT("Every map matches the height sample lattice"), Maps.IsValid(Field.GetHeights().Num()));
	const TArray<float>* Arrays[] = { &Field.GetHeights(), &Maps.SlopeDegrees, &Maps.Curvature, &Maps.Walkability, &Maps.Clearing, &Maps.MoisturePotential, &Maps.VegetationDensityPotential };
	const auto& Repeated = Repeat.GetMaps();
	const TArray<float>* RepeatArrays[] = { &Repeat.GetHeights(), &Repeated.SlopeDegrees, &Repeated.Curvature, &Repeated.Walkability, &Repeated.Clearing, &Repeated.MoisturePotential, &Repeated.VegetationDensityPotential };
	for (int32 M = 0; M < UE_ARRAY_COUNT(Arrays); ++M)
	{
		TestTrue(TEXT("Map generation repeats exactly"), *Arrays[M] == *RepeatArrays[M]);
		for (int32 Y = 0; Y < Settings.SamplesY; ++Y)
			for (int32 X = 0; X < Settings.SamplesX; ++X)
			{
				const int32 I = X + Y * Settings.SamplesX;
				const FVector2D XY((double(X) / (Settings.SamplesX - 1) - 0.5) * Settings.Width, (double(Y) / (Settings.SamplesY - 1) - 0.5) * Settings.Length);
				float Value;
				if (!TestTrue(TEXT("Map samples use exact heightmap coordinates"), Field.SampleLocalMap(EChopItTerrainMap(M), XY, Value) && FMath::Abs(Value - (*Arrays[M])[I]) < 0.001)) return false;
				if (M >= 3 && !TestTrue(TEXT("Potential masks are finite and in [0,1]"), FMath::IsFinite(Value) && Value >= 0 && Value <= 1)) return false;
			}
	}
	TestTrue(TEXT("Region categories repeat exactly"), Maps.Regions == Repeated.Regions);
	bool bSoftMembership = false;
	for (int32 I = 0; I < Maps.RegionBlends.Num(); ++I)
	{
		double Sum = 0; int32 Dominant = 0;
		for (int32 R = 0; R < FChopItTerrainRegionBlend::Count; ++R)
		{
			const float W = Maps.RegionBlends[I].Weights[R];
			if (!TestTrue(TEXT("Finite normalized memberships"), FMath::IsFinite(W) && W >= 0 && W <= 1)) return false;
			TestEqual(TEXT("Membership deterministic"), W, Repeated.RegionBlends[I].Weights[R]);
			Sum += W; bSoftMembership |= W > 0.01f && W < 0.99f;
			if (W > Maps.RegionBlends[I].Weights[Dominant]) Dominant = R;
		}
		if (!TestTrue(TEXT("Memberships sum to one"), FMath::Abs(Sum - 1.0) < 0.000001)) return false;
		if (!TestTrue(TEXT("Region Map is dominant membership"), Maps.Regions[I] == EChopItTerrainRegion(Dominant))) return false;
	}
	TestTrue(TEXT("Region transitions retain fractional memberships"), bSoftMembership);
	float Outside = 1;
	TestFalse(TEXT("Outside map query rejected"), Field.SampleLocalMap(EChopItTerrainMap::Slope, FVector2D(Settings.Width, 0), Outside));
	TestEqual(TEXT("Failed query resets value"), Outside, 0.0f);
	Settings.Regions.ClearingCoverage = 1;
	if (!FChopItTerrainGenerator::Generate(Settings, Repeat, Error)) return false;
	TestTrue(TEXT("Region controls preserve source geometry"), Field.GetHeights() == Repeat.GetHeights());
	TestFalse(TEXT("Coverage control changes clearing data"), Maps.Clearing == Repeat.GetMaps().Clearing);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainRegionControlsTest, "ChopIt.Terrain.Maps.ControlsAndValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainRegionControlsTest::RunTest(const FString& Parameters)
{
	FChopItTerrainRegionSettings Settings;
	const FIntPoint Samples(65, 49);
	const FVector2D Size(20000, 13000);
	const auto Flat = Surface(Samples, Size, [](double X, double Y) { return 500.0; });
	FChopItTerrainMaps Base, Changed;
	FString Error;
	if (!FChopItTerrainAnalysis::Build(Flat, Samples, Size, 0, 1337, Settings, Base, Error)) return false;
	Settings.ClearingCoverage = 0.6f;
	if (!FChopItTerrainAnalysis::Build(Flat, Samples, Size, 0, 1337, Settings, Changed, Error)) return false;
	TestTrue(TEXT("Increasing coverage increases clearings"), Average(Changed.Clearing) > Average(Base.Clearing));
	Settings.ClearingCoverage = 0.2f; Settings.RegionScale = 3000;
	if (!FChopItTerrainAnalysis::Build(Flat, Samples, Size, 0, 1337, Settings, Changed, Error)) return false;
	TestFalse(TEXT("Region scale changes patch distribution"), Base.Clearing == Changed.Clearing);
	Settings.RegionScale = 6000;
	if (!FChopItTerrainAnalysis::Build(Flat, Samples, Size, 0, 99, Settings, Changed, Error)) return false;
	TestFalse(TEXT("Seed changes patch distribution"), Base.Clearing == Changed.Clearing);
	Settings.TransitionWidth = 0.3f;
	if (!FChopItTerrainAnalysis::Build(Flat, Samples, Size, 0, 1337, Settings, Changed, Error)) return false;
	TestFalse(TEXT("Transition width changes soft masks"), Base.Clearing == Changed.Clearing);
	Settings = FChopItTerrainRegionSettings();
	Settings.MoistureHeightRange = 2000;
	if (!FChopItTerrainAnalysis::Build(Flat, Samples, Size, 0, 1337, Settings, Changed, Error)) return false;
	TestTrue(TEXT("Larger moisture range increases moisture away from water"), Average(Changed.MoisturePotential) > Average(Base.MoisturePotential));
	Settings.VegetationDensity = 0;
	if (!FChopItTerrainAnalysis::Build(Flat, Samples, Size, 0, 1337, Settings, Changed, Error)) return false;
	TestEqual(TEXT("Zero density suppresses all vegetation potential"), Average(Changed.VegetationDensityPotential), 0.0);
	Settings = FChopItTerrainRegionSettings();
	Settings.ClearingCoverage = 0;
	if (!FChopItTerrainAnalysis::Build(Flat, Samples, Size, 450, 1337, Settings, Base, Error)) return false;
	Settings.ShoreBlendHeight = 200;
	if (!FChopItTerrainAnalysis::Build(Flat, Samples, Size, 450, 1337, Settings, Changed, Error)) return false;
	TestTrue(TEXT("Wider shore blend reduces suitability near water"), Average(Changed.Walkability) < Average(Base.Walkability));
	const auto Ramp = Surface(Samples, FVector2D(2000, 1300), [](double X, double Y) { return 5000 + FMath::Tan(FMath::DegreesToRadians(10.0)) * X; });
	Settings = FChopItTerrainRegionSettings(); Settings.ClearingCoverage = 0; Settings.FlatSlopeDegrees = 15;
	if (!FChopItTerrainAnalysis::Build(Ramp, Samples, FVector2D(2000, 1300), 0, 1, Settings, Changed, Error)) return false;
	TestTrue(TEXT("Flat slope threshold changes classification"), Changed.Regions[0] == EChopItTerrainRegion::FlatForest);
	Settings.FlatSlopeDegrees = 5; Settings.SteepSlopeDegrees = 8; Settings.MaxWalkableSlopeDegrees = 5;
	if (!FChopItTerrainAnalysis::Build(Ramp, Samples, FVector2D(2000, 1300), 0, 1, Settings, Changed, Error)) return false;
	TestTrue(TEXT("Steep slope threshold changes classification"), Changed.Regions[0] == EChopItTerrainRegion::SteepHills);
	TestEqual(TEXT("Walkable slope threshold restricts suitable terrain"), Average(Changed.Walkability), 0.0);
	const auto Previous = Changed.Clearing;
	Settings.FlatSlopeDegrees = Settings.SteepSlopeDegrees;
	TestFalse(TEXT("Unordered slope thresholds rejected"), FChopItTerrainAnalysis::Build(Flat, Samples, Size, 0, 1, Settings, Changed, Error));
	TestTrue(TEXT("Failure preserves map data"), Previous == Changed.Clearing);
	Settings = FChopItTerrainRegionSettings();
	TArray<float> Invalid = Flat;
	Invalid[0] = std::numeric_limits<float>::quiet_NaN();
	TestFalse(TEXT("Nonfinite samples rejected"), FChopItTerrainAnalysis::Build(Invalid, Samples, Size, 0, 1, Settings, Changed, Error));
	TestFalse(TEXT("Wrong grid dimensions rejected"), FChopItTerrainAnalysis::Build(Flat, FIntPoint(64, 49), Size, 0, 1, Settings, Changed, Error));
	TestFalse(TEXT("Validation provides diagnosis"), Error.IsEmpty());
	return true;
}
#endif
