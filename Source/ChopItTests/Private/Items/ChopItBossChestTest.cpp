#include "Framework/ChopItGameState.h"
#include "Rewards/ChopItBossRewardChest.h"
#include "Spawning/ChopItEliteEncounterComponent.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Enemies/ChopItEnemyCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "Engine/Texture2D.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItBossChestSpawnTest,
	"ChopIt.Items.BossChestSpawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItBossChestSpawnTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	AChopItGameState* State = World->SpawnActor<AChopItGameState>();
	World->SetGameState(State);
	World->BeginPlay();
	if (!State->HasActorBegunPlay()) State->DispatchBeginPlay();
	AActor* DefeatedElite = World->SpawnActor<AActor>();
	DefeatedElite->SetActorLocation(FVector(250.f, -150.f, 120.f));
	UChopItEliteEncounterComponent* Encounter = State->FindComponentByClass<UChopItEliteEncounterComponent>();
	TestNotNull(TEXT("Existing encounter component"), Encounter);
	if (Encounter)
	{
		Encounter->OnEliteDefeated.Broadcast(DefeatedElite, nullptr);
		int32 Chests = 0;
		for (TActorIterator<AChopItBossRewardChest> It(World); It; ++It)
		{
			++Chests;
			TestTrue(TEXT("Chest spawns at the elite's XY position"),
				FVector::DistSquared2D(It->GetActorLocation(), DefeatedElite->GetActorLocation()) < 1.f);
		}
		TestEqual(TEXT("One chest from one elite death"), Chests, 1);
	}
	TestNotNull(TEXT("Arrival Niagara asset"), LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/ChopIt/Items/Chest/NS_BossChest_Appear.NS_BossChest_Appear")));
	TestNotNull(TEXT("Opening Niagara asset"), LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/ChopIt/Items/Chest/NS_BossChest_Open.NS_BossChest_Open")));
	TestNotNull(TEXT("Idle Niagara asset"), LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/ChopIt/Items/Chest/NS_BossChest_Idle.NS_BossChest_Idle")));
	for (const TCHAR* Icon : {TEXT("Heartwood"), TEXT("VampireStick"), TEXT("Wormwood")})
	{
		const FString AssetName = FString(TEXT("T_Stick_")) + Icon;
		const FString Path = FString::Printf(TEXT("/Game/ChopIt/Art/Sticks/%s.%s"), *AssetName, *AssetName);
		TestNotNull(FString::Printf(TEXT("Imported icon %s"), Icon), LoadObject<UTexture2D>(nullptr, *Path));
	}
	World->DestroyWorld(false);
	return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItEliteSpawnTest,
	"ChopIt.Items.EliteSpawnWithoutGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItEliteSpawnTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	AChopItGameState* State = World->SpawnActor<AChopItGameState>();
	World->SetGameState(State);
	World->BeginPlay();
	if (!State->HasActorBegunPlay()) State->DispatchBeginPlay();
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	World->AddController(Controller);
	APawn* Pawn = World->SpawnActor<APawn>();
	Pawn->SetActorLocation(FVector(0.f, 0.f, 100.f));
	Controller->Possess(Pawn);
	UChopItCycleStateMachineComponent* Cycle = State->GetCycleStateMachine();
	TestNotNull(TEXT("Cycle component"), Cycle);
	if (Cycle)
	{
		TestTrue(TEXT("Day to dusk"), Cycle->TransitionForAutomation(EChopItCyclePhase::Dusk));
		TestTrue(TEXT("Dusk to night"), Cycle->TransitionForAutomation(EChopItCyclePhase::Night));
		TestTrue(TEXT("Night to elite"), Cycle->TransitionForAutomation(EChopItCyclePhase::Elite));
		int32 Elites = 0;
		for (TActorIterator<AChopItEnemyCharacter> It(World); It; ++It) ++Elites;
		TestEqual(TEXT("Elite spawns even when there is no WorldStatic ground"), Elites, 1);
	}
	World->DestroyWorld(false);
	return true;
}
#endif
