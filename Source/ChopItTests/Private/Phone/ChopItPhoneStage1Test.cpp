#include "Events/ChopItSpecialEventPlacement.h"
#include "Interaction/ChopItInteractionComponent.h"
#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneSpawnConfig.h"
#include "Phone/ChopItPhoneStateComponent.h"
#include "Framework/ChopItAllFeaturesGameMode.h"
#include "Terrain/ChopItTerrainActor.h"
#include "Terrain/ChopItTerrainDataAsset.h"
#include "Terrain/ChopItTerrainProjectSettings.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItPhoneStage1Test, "ChopIt.Phone.Stage1.PlacementAndState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhoneStage1Test::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };

	UChopItTerrainDataAsset* TerrainRecipe = NewObject<UChopItTerrainDataAsset>();
	TerrainRecipe->Terrain.Width = 4000.0;
	TerrainRecipe->Terrain.Length = 4000.0;
	TerrainRecipe->Terrain.SamplesX = 33;
	TerrainRecipe->Terrain.SamplesY = 33;
	TerrainRecipe->Terrain.HeightVariation = 0.0f;
	TerrainRecipe->Terrain.TargetMeanHeight = 100.0f;
	TerrainRecipe->Terrain.WaterLevel = 0.0f;
	TerrainRecipe->Mesh.PolygonDetail = 0;
	FString Error;
	AChopItTerrainActor* Terrain = TerrainRecipe->SpawnTerrain(World, FTransform::Identity, 731, Error);
	if (!TestNotNull(TEXT("Flat seeded terrain"), Terrain)) { AddError(Error); return false; }

	FChopItSpecialEventPlacementRules Rules;
	Rules.MinDistanceFromStart = 350.0f;
	Rules.MaxDistanceFromStart = 700.0f;
	Rules.MinWalkability = 0.0f;
	Rules.HeightAboveWater = 10.0f;
	Rules.FootprintRadius = 40.0f;
	Rules.ClearanceHalfHeight = 80.0f;
	Rules.RouteSampleStep = 100.0f;
	Rules.MaxAttempts = 16;
	FRandomStream FirstRandom(1234), SecondRandom(1234);
	FVector First, Second;
	const FVector Start(0, 0, 100);
	TestTrue(TEXT("First event location exists"),
		FChopItSpecialEventPlacement::FindLocation(*World, *Terrain, Start, Rules, FirstRandom, {}, First));
	TestTrue(TEXT("Same seed finds a location"),
		FChopItSpecialEventPlacement::FindLocation(*World, *Terrain, Start, Rules, SecondRandom, {}, Second));
	TestTrue(TEXT("Same seed and world give the same location"), First.Equals(Second, 0.01f));
	TestTrue(TEXT("Location respects minimum distance"), FVector::Dist2D(Start, First) >= Rules.MinDistanceFromStart);
	TestTrue(TEXT("Location respects maximum distance"), FVector::Dist2D(Start, First) <= Rules.MaxDistanceFromStart);
	FVector Rejected;
	FRandomStream ThirdRandom(1234);
	Rules.MinDistanceBetweenEvents = 10000.0f;
	TestFalse(TEXT("Existing event exclusion rejects all candidates"),
		FChopItSpecialEventPlacement::FindLocation(*World, *Terrain, Start, Rules, ThirdRandom, {First}, Rejected));
	Rules.MinDistanceBetweenEvents = 0.0f;
	Rules.HeightAboveWater = 200.0f;
	FRandomStream FourthRandom(1234);
	TestFalse(TEXT("Water clearance rejects submerged candidates"),
		FChopItSpecialEventPlacement::FindLocation(*World, *Terrain, Start, Rules, FourthRandom, {}, Rejected));
	Terrain->TerrainSettings.SpawnFlatRadius = 1000.0f;
	Rules.MinWalkability = 1.0f;
	FRandomStream ClearingRandom(1234);
	FVector ClearingLocation;
	TestTrue(TEXT("Spawn clearing remains usable when water and walkability metadata reject it"),
		FChopItSpecialEventPlacement::FindLocation(*World, *Terrain, Start, Rules, ClearingRandom, {}, ClearingLocation));
	Terrain->TerrainSettings.SpawnFlatRadius = 0.0f;
	Rules.MinWalkability = 0.0f;
	AActor* RouteBlocker = World->SpawnActor<AActor>();
	USphereComponent* BlockerBody = NewObject<USphereComponent>(RouteBlocker);
	RouteBlocker->SetRootComponent(BlockerBody);
	RouteBlocker->AddInstanceComponent(BlockerBody);
	BlockerBody->InitSphereRadius(250.0f);
	BlockerBody->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BlockerBody->SetCollisionObjectType(ECC_WorldStatic);
	BlockerBody->SetCollisionResponseToAllChannels(ECR_Block);
	BlockerBody->RegisterComponent();
	RouteBlocker->SetActorLocation(FMath::Lerp(Start, First, 0.5f));
	Rules.HeightAboveWater = 10.0f;
	Rules.MaxAttempts = 1;
	FRandomStream BlockedRandom(1234);
	TestFalse(TEXT("Authored obstacle blocks the sampled route"),
		FChopItSpecialEventPlacement::FindLocation(*World, *Terrain, Start, Rules, BlockedRandom, {}, Rejected));
	RouteBlocker->Destroy();

	AChopItMysteryPhone* Phone = World->SpawnActor<AChopItMysteryPhone>(First, FRotator::ZeroRotator);
	FActorSpawnParameters InteractorParams;
	InteractorParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APawn* Pawn = World->SpawnActor<ADefaultPawn>(ADefaultPawn::StaticClass(),
		First + FVector(100, 0, 90), FRotator::ZeroRotator, InteractorParams);
	if (!TestNotNull(TEXT("Phone actor"), Phone) || !TestNotNull(TEXT("Interactor"), Pawn)) return false;
	Pawn->SetActorLocation(Phone->GetActorLocation() + FVector(100, 0, 90), false, nullptr, ETeleportType::TeleportPhysics);
	TestTrue(TEXT("Interactor is within range"),
		FVector::Dist(Phone->GetActorLocation(), Pawn->GetActorLocation()) < Phone->InteractionRange);
	TestFalse(TEXT("Invalid state skip is rejected"), Phone->GetPhoneState()->TryTransition(EChopItPhoneState::Dialogue));
	TestTrue(TEXT("Direct interaction validation"), Phone->CanInteract_Implementation(Pawn));
	TestTrue(TEXT("Phone implements interaction interface"), Phone->GetClass()->ImplementsInterface(UChopItInteractable::StaticClass()));
	UChopItInteractionComponent* Interaction = NewObject<UChopItInteractionComponent>(Pawn);
	Pawn->AddInstanceComponent(Interaction);
	Interaction->RegisterComponent();
	TestTrue(TEXT("Existing input interaction answers the phone"), Interaction->TryInteract());
	TestEqual(TEXT("Answer state"), Phone->GetPhoneState()->GetState(), EChopItPhoneState::Answered);
	TestFalse(TEXT("Duplicate answer is rejected"), Interaction->TryInteract());
	TestTrue(TEXT("Transform transition"), Phone->GetPhoneState()->TryTransition(EChopItPhoneState::Transforming));
	TestTrue(TEXT("Dialogue transition"), Phone->GetPhoneState()->TryTransition(EChopItPhoneState::Dialogue));
	TestTrue(TEXT("Completion transition"), Phone->GetPhoneState()->TryTransition(EChopItPhoneState::Completed));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItPhoneGameModeTest, "ChopIt.Phone.Stage1.FullFeatureWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhoneGameModeTest::RunTest(const FString& Parameters)
{
	UChopItTerrainProjectSettings* Settings = GetMutableDefault<UChopItTerrainProjectSettings>();
	const auto SavedRecipe = Settings->DefaultTerrain;
	ON_SCOPE_EXIT { Settings->DefaultTerrain = SavedRecipe; };
	UChopItTerrainDataAsset* Recipe = NewObject<UChopItTerrainDataAsset>(Settings);
	Recipe->Terrain.SamplesX = 33;
	Recipe->Terrain.SamplesY = 33;
	Recipe->Terrain.HeightVariation = 0.0f;
	Recipe->Terrain.TargetMeanHeight = 100.0f;
	Recipe->Terrain.WaterLevel = 0.0f;
	Recipe->Mesh.PolygonDetail = 0;
	Settings->DefaultTerrain = Recipe;

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	World->SpawnActor<APlayerStart>(FVector::ZeroVector, FRotator::ZeroRotator);
	AChopItAllFeaturesGameMode* Mode = World->SpawnActor<AChopItAllFeaturesGameMode>();
	if (!TestNotNull(TEXT("All-features GameMode"), Mode)) return false;
	TestFalse(TEXT("Terrain generation is disabled by default"), Mode->bGenerateTerrainAtRuntime);
	Mode->bGenerateTerrainAtRuntime = true;
	UChopItPhoneSpawnConfig* SpawnConfig = Mode->PhoneSpawnConfig.LoadSynchronous();
	if (!TestNotNull(TEXT("Phone spawn configuration"), SpawnConfig)) return false;
	UChopItPhoneSpawnConfig* TestSpawnConfig = DuplicateObject<UChopItPhoneSpawnConfig>(SpawnConfig, Mode);
	TestSpawnConfig->Placement.MinDistanceFromStart = 100000.0f;
	TestSpawnConfig->Placement.MaxDistanceFromStart = 110000.0f;
	Mode->PhoneSpawnConfig = TestSpawnConfig;
	FString Error;
	Mode->InitGame(TEXT("PhoneStage1Test"), TEXT(""), Error);
	TestTrue(TEXT("Terrain generation succeeds"), Error.IsEmpty() && IsValid(Mode->GeneratedTerrain));
	TestEqual(TEXT("Phone spawns in the clearing when the broad search cannot find terrain"), Mode->SpawnedPhones.Num(), 1);
	if (Mode->SpawnedPhones.Num() == 1)
	{
		TestTrue(TEXT("Spawned phone uses the configured Blueprint"),
			Mode->SpawnedPhones[0]->GetClass()->GetName().Contains(TEXT("BP_MysteryPhone")));
		TestEqual(TEXT("Phone begins available"), Mode->SpawnedPhones[0]->GetPhoneState()->GetState(),
			EChopItPhoneState::Available);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		APawn* Pawn = World->SpawnActor<ADefaultPawn>(ADefaultPawn::StaticClass(),
			Mode->SpawnedPhones[0]->GetActorLocation() + FVector(100, 0, 90), FRotator::ZeroRotator, Params);
		if (TestNotNull(TEXT("Blueprint phone interactor"), Pawn))
		{
			Pawn->SetActorLocation(Mode->SpawnedPhones[0]->GetActorLocation() + FVector(100, 0, 90));
			UChopItInteractionComponent* Interaction = NewObject<UChopItInteractionComponent>(Pawn);
			Pawn->AddInstanceComponent(Interaction);
			Interaction->RegisterComponent();
			TestTrue(TEXT("Blueprint phone receives the existing interaction"), Interaction->TryInteract());
		}
	}
	return true;
}
#endif
