#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Cycle/ChopItRunStateComponent.h"
#include "Cycle/ChopItWorldPresentationComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Economy/ChopItCabinHub.h"
#include "Economy/ChopItDayDefinition.h"
#include "Economy/ChopItQuotaMachine.h"
#include "Engine/World.h"
#include "Framework/ChopItGameState.h"
#include "Interaction/ChopItInteractable.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase5TransitionTableTest,
	"ChopIt.Phase5.TransitionTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase5TransitionTableTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Bootstrap enters Day"), UChopItCycleStateMachineComponent::IsTransitionAllowed(EChopItCyclePhase::Bootstrap, EChopItCyclePhase::Day));
	TestTrue(TEXT("Day enters Dusk"), UChopItCycleStateMachineComponent::IsTransitionAllowed(EChopItCyclePhase::Day, EChopItCyclePhase::Dusk));
	TestTrue(TEXT("Dusk enters Night"), UChopItCycleStateMachineComponent::IsTransitionAllowed(EChopItCyclePhase::Dusk, EChopItCyclePhase::Night));
	TestTrue(TEXT("Night enters Elite"), UChopItCycleStateMachineComponent::IsTransitionAllowed(EChopItCyclePhase::Night, EChopItCyclePhase::Elite));
	TestTrue(TEXT("Elite enters Resolution"), UChopItCycleStateMachineComponent::IsTransitionAllowed(EChopItCyclePhase::Elite, EChopItCyclePhase::Resolution));
	TestTrue(TEXT("Resolution can start next Day"), UChopItCycleStateMachineComponent::IsTransitionAllowed(EChopItCyclePhase::Resolution, EChopItCyclePhase::Day));
	TestTrue(TEXT("Dusk may fail into Death"), UChopItCycleStateMachineComponent::IsTransitionAllowed(EChopItCyclePhase::Dusk, EChopItCyclePhase::Death));
	TestFalse(TEXT("Day cannot skip directly to Night"), UChopItCycleStateMachineComponent::IsTransitionAllowed(EChopItCyclePhase::Day, EChopItCyclePhase::Night));
	TestFalse(TEXT("Death is terminal"), UChopItCycleStateMachineComponent::IsTransitionAllowed(EChopItCyclePhase::Death, EChopItCyclePhase::Day));
	TestEqual(TEXT("Paid dusk deadline enters Night"), UChopItCycleStateMachineComponent::ResolveDuskDeadline(true), EChopItCyclePhase::Night);
	TestEqual(TEXT("Unpaid dusk deadline still enters Night"), UChopItCycleStateMachineComponent::ResolveDuskDeadline(false), EChopItCyclePhase::Night);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase5LeverRulesTest,
	"ChopIt.Phase5.LeverRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase5LeverRulesTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Lever rejected during Day"), UChopItCycleStateMachineComponent::CanAcceptLever(EChopItCyclePhase::Day, true, true));
	TestFalse(TEXT("Lever rejected with unpaid quota"), UChopItCycleStateMachineComponent::CanAcceptLever(EChopItCyclePhase::Night, false, true));
	TestTrue(TEXT("Paid quota enables lever immediately during Night"), UChopItCycleStateMachineComponent::CanAcceptLever(EChopItCyclePhase::Night, true, false));
	TestTrue(TEXT("Lever remains available later during Night"), UChopItCycleStateMachineComponent::CanAcceptLever(EChopItCyclePhase::Night, true, true));
	TestTrue(TEXT("Quota machine implements explicit interaction"), GetDefault<AChopItQuotaMachine>()->Implements<UChopItInteractable>());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase5GenerationStressTest,
	"ChopIt.Phase5.GenerationStress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase5GenerationStressTest::RunTest(const FString& Parameters)
{
	UChopItCycleStateMachineComponent* Cycle = NewObject<UChopItCycleStateMachineComponent>();
	TestFalse(TEXT("Cycle component has no Tick"), Cycle->PrimaryComponentTick.bCanEverTick);
	for (int32 Iteration = 0; Iteration < 20; ++Iteration)
	{
		TestTrue(TEXT("Cycle starts Day"), Cycle->StartCycle());
		const uint32 DayGeneration = Cycle->GetPhaseGeneration();
		TestTrue(TEXT("Day advances to Dusk"), Cycle->TransitionForAutomation(EChopItCyclePhase::Dusk));
		TestFalse(TEXT("Day callback becomes stale"), Cycle->IsGenerationCurrentForAutomation(DayGeneration));
		TestTrue(TEXT("Dusk advances to Night"), Cycle->TransitionForAutomation(EChopItCyclePhase::Night));
		TestTrue(TEXT("Night advances to Elite"), Cycle->TransitionForAutomation(EChopItCyclePhase::Elite));
		TestTrue(TEXT("Elite advances to Resolution"), Cycle->TransitionForAutomation(EChopItCyclePhase::Resolution));
	}
	TestEqual(TEXT("Twenty cycles end in Resolution"), Cycle->GetCurrentPhase(), EChopItCyclePhase::Resolution);
	TestEqual(TEXT("Every transition increments generation"), Cycle->GetPhaseGeneration(), uint32(100));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase5RunStateAndTimingsTest,
	"ChopIt.Phase5.RunStateAndTimings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase5RunStateAndTimingsTest::RunTest(const FString& Parameters)
{
	UChopItRunStateComponent* RunState = NewObject<UChopItRunStateComponent>();
	RunState->InitializeRun(1);
	RunState->MarkCycleCompleted();
	RunState->MarkQuotaFailed();
	TestEqual(TEXT("Terminal result cannot be overwritten"), RunState->GetResult(), EChopItRunResult::CycleCompleted);
	RunState->AdvanceDay();
	TestEqual(TEXT("Advancing increments day"), RunState->GetDayNumber(), 2);
	TestEqual(TEXT("Advancing resets provisional result"), RunState->GetResult(), EChopItRunResult::None);

	UChopItDayDefinition* Day = NewObject<UChopItDayDefinition>();
	Day->DayDuration = -10.0f;
	Day->DuskMinimumDuration = 5.0f;
	Day->DuskHardDeadline = 2.0f;
	Day->NightMinimumDuration = -1.0f;
	Day->ElitePlaceholderDuration = 0.0f;
	Day->ResolutionDuration = 0.0f;
	UChopItCycleStateMachineComponent* Cycle = NewObject<UChopItCycleStateMachineComponent>();
	Cycle->ConfigureFromDay(Day);
	TestTrue(TEXT("Day duration is clamped positive"), Cycle->GetTimings().DayDuration >= 0.1f);
	TestTrue(TEXT("Hard deadline cannot precede Dusk minimum"), Cycle->GetTimings().DuskHardDeadline >= Cycle->GetTimings().DuskMinimumDuration);
	TestEqual(TEXT("Night minimum accepts zero"), Cycle->GetTimings().NightMinimumDuration, 0.0f);
	TestTrue(TEXT("Elite placeholder remains positive"), Cycle->GetTimings().ElitePlaceholderDuration >= 0.1f);
	TestTrue(TEXT("Resolution duration remains positive"), Cycle->GetTimings().ResolutionDuration >= 0.1f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase5AssetsAndMapTest,
	"ChopIt.Phase5.AssetsAndMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase5AssetsAndMapTest::RunTest(const FString& Parameters)
{
	const UChopItDayDefinition* Day = LoadObject<UChopItDayDefinition>(
		nullptr, TEXT("/Game/ChopIt/Economy/Days/DA_Day_01.DA_Day_01"));
	TestNotNull(TEXT("Day definition contains cycle timings"), Day);
	if (Day)
	{
		TestEqual(TEXT("Day lasts five seconds for the current recording loop"), Day->DayDuration, 5.0f);
		TestEqual(TEXT("Dusk lasts exactly two seconds"), Day->DuskHardDeadline, 2.0f);
		TestTrue(TEXT("Dusk hard deadline follows minimum"), Day->DuskHardDeadline >= Day->DuskMinimumDuration);
		TestTrue(TEXT("Night has a minimum duration"), Day->NightMinimumDuration > 0.0f);
	}

	const FString MapPackageName = TEXT("/Game/ChopIt/World/Maps/L_Test_Cycle");
	TestTrue(TEXT("Cycle map exists"), FPackageName::DoesPackageExist(MapPackageName));
	UPackage* Package = LoadPackage(nullptr, *MapPackageName, LOAD_None);
	TestNotNull(TEXT("Cycle map loads"), Package);
	TestNotNull(TEXT("Cycle map world exists"), Package ? FindObject<UWorld>(Package, TEXT("L_Test_Cycle")) : nullptr);

	const AChopItGameState* GameState = GetDefault<AChopItGameState>();
	TestNotNull(TEXT("GameState owns run state"), GameState->GetRunStateComponent());
	TestNotNull(TEXT("GameState owns cycle FSM"), GameState->GetCycleStateMachine());
	TestFalse(TEXT("FSM has no Tick"), GameState->GetCycleStateMachine()->PrimaryComponentTick.bCanEverTick);
	TestTrue(TEXT("Presentation can tick only while blending"), GetDefault<UChopItWorldPresentationComponent>()->PrimaryComponentTick.bCanEverTick);
	const AChopItCabinHub* Cabin = GetDefault<AChopItCabinHub>();
	TestNotNull(TEXT("Cabin owns visible guidance light"), Cabin->FindComponentByClass<UPointLightComponent>());
	TestNotNull(TEXT("Cabin owns visible guidance label"), Cabin->FindComponentByClass<UTextRenderComponent>());
	return true;
}

#endif
