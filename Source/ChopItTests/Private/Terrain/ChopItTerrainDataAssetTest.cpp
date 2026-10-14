#include "Terrain/ChopItTerrainDataAsset.h"
#include "Terrain/ChopItTerrainActor.h"
#include "Terrain/ChopItTerrainProjectSettings.h"
#include "Terrain/ChopItTerrainTestGameMode.h"
#include "Terrain/ChopItTerrainMeshBuilder.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainDataAssetTest,
	"ChopIt.Terrain.Data.SeedlessAssetRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainDataAssetTest::RunTest(const FString& Parameters)
{
	UChopItTerrainDataAsset* Asset = NewObject<UChopItTerrainDataAsset>();
	TestNull(TEXT("Profile has no serialized Seed property"), FChopItTerrainProfile::StaticStruct()->FindPropertyByName(TEXT("Seed")));
	FChopItTerrainSettings Source;
	Source.Seed = 943;
	Source.SamplesX = 33; Source.SamplesY = 25;
	Source.HillStrength = 1.6f;
	Source.Regions.ClearingCoverage = 0.33f;
	Source.NoiseLayers.AddDefaulted();
	Asset->Terrain.CopyFrom(Source);
	Asset->Mesh.PolygonDetail = 0;
	FChopItTerrainSettings First = Asset->MakeSettings(111), Second = Asset->MakeSettings(222);
	TestEqual(TEXT("Runtime seed one"), First.Seed, 111);
	TestEqual(TEXT("Runtime seed two"), Second.Seed, 222);
	TestEqual(TEXT("Profile copies height resolution"), First.SamplesX, Source.SamplesX);
	TestEqual(TEXT("Profile copies region controls"), First.Regions.ClearingCoverage, Source.Regions.ClearingCoverage);
	TestEqual(TEXT("Profile copies array settings"), First.NoiseLayers.Num(), 1);
	First.Seed = Source.Seed;
	TestTrue(TEXT("All non-seed controls round-trip exactly"),
		FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&First, &Source, 0));
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	FString Error;
	AChopItTerrainActor* Actor = Asset->SpawnTerrain(World, FTransform::Identity, 111, Error);
	if (!TestNotNull(TEXT("Spawn terrain from recipe"), Actor)) { AddError(Error); return false; }
	TestEqual(TEXT("Spawn uses runtime seed"), Actor->TerrainSettings.Seed, 111);
	const TArray<float> InitialHeights = Actor->GetHeightField().GetHeights();
	TestTrue(TEXT("Generate on existing actor"), Asset->GenerateOnActor(Actor, 222, Error));
	TestEqual(TEXT("Regeneration uses new runtime seed"), Actor->TerrainSettings.Seed, 222);
	TestFalse(TEXT("Different run seeds change heights"), InitialHeights == Actor->GetHeightField().GetHeights());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItDefaultTerrainWorldTest,
	"ChopIt.Terrain.Data.DefaultWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItDefaultTerrainWorldTest::RunTest(const FString& Parameters)
{
	UChopItTerrainProjectSettings* Settings = GetMutableDefault<UChopItTerrainProjectSettings>();
	const auto OriginalDefault = Settings->DefaultTerrain;
	ON_SCOPE_EXIT { Settings->DefaultTerrain = OriginalDefault; };
	TestNotNull(TEXT("Configured project recipe loads"), Settings->DefaultTerrain.LoadSynchronous());
	UChopItTerrainDataAsset* Recipe = NewObject<UChopItTerrainDataAsset>();
	Recipe->Terrain.SamplesX = 17;
	Recipe->Terrain.SamplesY = 17;
	Recipe->Terrain.HillStrength = 1.7f;
	Recipe->Mesh.PolygonDetail = 0;
	Settings->DefaultTerrain = Recipe;
	int32 FirstSeed = 0;
	TArray<float> FirstHeights;
	const FChopItTerrainSettings OriginalRecipe = Recipe->MakeSettings(0);
	FChopItTerrainSettings ExpectedRunSettings = OriginalRecipe;
	ExpectedRunSettings.SpawnFlatCenter = FVector2D::ZeroVector;
	const FIntPoint RunMeshCells = FChopItTerrainMeshBuilder::GetCellCounts(
		FVector2D(ExpectedRunSettings.Width, ExpectedRunSettings.Length), Recipe->Mesh.PolygonDetail);
	ExpectedRunSettings.SpawnFlatRadius = FMath::Max(ExpectedRunSettings.SpawnFlatRadius, 1000.0f)
		+ float(FMath::Sqrt(FMath::Square(ExpectedRunSettings.Width / RunMeshCells.X)
			+ FMath::Square(ExpectedRunSettings.Length / RunMeshCells.Y)));
	ExpectedRunSettings.SpawnFlatBlendWidth = FMath::Max(ExpectedRunSettings.SpawnFlatBlendWidth, 500.0f);
	for (int32 Run = 0; Run < 2; ++Run)
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
		AChopItTerrainTestGameMode* Mode = World->SpawnActor<AChopItTerrainTestGameMode>();
		Mode->bGenerateTerrainAtRuntime = true;
		FString Error;
		// A stale seed option must not make consecutive Play sessions identical.
		Mode->InitGame(TEXT("TerrainDefaultTest"), TEXT("?Seed=424242"), Error);
		TestTrue(TEXT("Default world starts without an error"), Error.IsEmpty());
		if (!TestNotNull(TEXT("Game mode generates configured recipe"), Mode->GeneratedTerrain.Get())) return false;
		FChopItTerrainSettings Generated = Mode->GeneratedTerrain->TerrainSettings;
		TestEqual(TEXT("Generated terrain receives new run seed"), Generated.Seed, Mode->RuntimeSeed);
		Generated.Seed = 0;
		TestTrue(TEXT("Recipe parameters and the runtime spawn area are applied"),
			FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&Generated, &ExpectedRunSettings, 0));
		TestTrue(TEXT("Runtime world has terrain geometry"), !Mode->GeneratedTerrain->GetTerrainChunks().IsEmpty());
		if (Run == 0)
		{
			FirstSeed = Mode->RuntimeSeed;
			FirstHeights = Mode->GeneratedTerrain->GetHeightField().GetHeights();
		}
		else
		{
			TestNotEqual(TEXT("Consecutive Play sessions choose different seeds"), Mode->RuntimeSeed, FirstSeed);
			TestFalse(TEXT("Consecutive Play sessions generate different terrain"), FirstHeights == Mode->GeneratedTerrain->GetHeightField().GetHeights());
		}
	}
	const FChopItTerrainSettings RecipeAfterPlaying = Recipe->MakeSettings(0);
	TestTrue(TEXT("Playing leaves the recipe unchanged"), FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&RecipeAfterPlaying, &OriginalRecipe, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItDefaultTerrainCollisionTest,
	"ChopIt.Terrain.Data.DefaultAssetCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItDefaultTerrainCollisionTest::RunTest(const FString& Parameters)
{
	UChopItTerrainDataAsset* Recipe = GetDefault<UChopItTerrainProjectSettings>()->DefaultTerrain.LoadSynchronous();
	if (!TestNotNull(TEXT("Default terrain recipe"), Recipe)) return false;
	TestTrue(TEXT("Gameplay recipe enables collision"), Recipe->Mesh.bEnableCollision);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	FString Error;
	FChopItTerrainSettings RunSettings = Recipe->MakeSettings(9123);
	RunSettings.SpawnFlatCenter = FVector2D(750.0, 0.0);
	const FIntPoint MeshCells = FChopItTerrainMeshBuilder::GetCellCounts(
		FVector2D(RunSettings.Width, RunSettings.Length), Recipe->Mesh.PolygonDetail);
	RunSettings.SpawnFlatRadius = 1000.0f + float(FMath::Sqrt(
		FMath::Square(RunSettings.Width / MeshCells.X) + FMath::Square(RunSettings.Length / MeshCells.Y)));
	RunSettings.SpawnFlatBlendWidth = 500.0f;
	AChopItTerrainActor* Terrain = Recipe->SpawnTerrainWithSettings(World, FTransform::Identity, RunSettings, Error);
	if (!TestNotNull(TEXT("Default terrain generated"), Terrain)) { AddError(Error); return false; }
	float GroundHeight = 0.0f;
	const FVector SpawnCenter(750.0, 0.0, 0.0);
	if (!TestTrue(TEXT("Ground can be sampled"), Terrain->SampleWorldHeight(SpawnCenter, GroundHeight))) return false;
	for (const FVector Offset : { FVector(0, 0, 0), FVector(400, 0, 0), FVector(-400, 0, 0), FVector(0, 400, 0), FVector(0, -400, 0) })
	{
		float NearbyHeight = 0.0f;
		TestTrue(TEXT("Flat spawn point can be sampled"), Terrain->SampleWorldHeight(SpawnCenter + Offset, NearbyHeight));
		TestTrue(TEXT("Spawn surface is level"), FMath::Abs(NearbyHeight - GroundHeight) < 0.01f);
	}
	const FVector Start(750, 0, GroundHeight + 1000.0f), End(750, 0, GroundHeight - 1000.0f);
	FHitResult Hit;
	TestTrue(TEXT("Player capsule sweep hits the generated ground"),
		World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(42.0f, 88.0f)));
	TestTrue(TEXT("Collision belongs to the generated terrain"), Hit.GetActor() == Terrain);
	for (const FVector Offset : { FVector(0, 0, 0), FVector(400, 0, 0), FVector(-400, 0, 0), FVector(0, 400, 0), FVector(0, -400, 0), FVector(800, 0, 0), FVector(0, 800, 0) })
	{
		FHitResult SurfaceHit;
		if (TestTrue(TEXT("Generated spawn mesh has collision"), World->LineTraceSingleByChannel(SurfaceHit, Start + Offset, End + Offset, ECC_Visibility)))
		{
			TestTrue(TEXT("Physical spawn surface is level"), FMath::Abs(SurfaceHit.ImpactPoint.Z - GroundHeight) < 0.1f);
		}
	}
	return true;
}
#endif
