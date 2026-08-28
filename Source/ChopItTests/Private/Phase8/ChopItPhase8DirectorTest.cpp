// Live Coding cannot add the newly introduced ChopItAI link dependency to an
// already loaded test DLL. The same tests compile and run in normal CI builds.
#if WITH_DEV_AUTOMATION_TESTS && !WITH_LIVE_CODING

#include "Enemies/ChopItEnemyDefinition.h"
#include "Misc/AutomationTest.h"
#include "Spawning/ChopItEnemyDirectorComponent.h"
#include "Spawning/ChopItEnemyDirectorDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItPhase8BudgetTest, "ChopIt.Phase8.DirectorBudget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItPhase8BudgetTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Base wave budget"), UChopItEnemyDirectorComponent::CalculateWaveBudget(3, 1, 0), 3);
	TestEqual(TEXT("Growth is deterministic"), UChopItEnemyDirectorComponent::CalculateWaveBudget(3, 2, 4), 11);
	TestEqual(TEXT("Negative inputs are safe"), UChopItEnemyDirectorComponent::CalculateWaveBudget(-3, -1, -2), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItPhase8AssetsTest, "ChopIt.Phase8.Assets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItPhase8AssetsTest::RunTest(const FString& Parameters)
{
	const UChopItEnemyDefinition* Basic = LoadObject<UChopItEnemyDefinition>(nullptr, TEXT("/Game/ChopIt/AI/Enemies/DA_Enemy_Tree.DA_Enemy_Tree"));
	const UChopItEnemyDefinition* Fast = LoadObject<UChopItEnemyDefinition>(nullptr, TEXT("/Game/ChopIt/AI/Enemies/DA_Enemy_FastTree.DA_Enemy_FastTree"));
	const UChopItEnemyDirectorDefinition* Director = LoadObject<UChopItEnemyDirectorDefinition>(nullptr, TEXT("/Game/ChopIt/AI/Directors/DA_Director_Day01.DA_Director_Day01"));
	TestNotNull(TEXT("Basic enemy asset exists"), Basic);
	TestNotNull(TEXT("Fast enemy asset exists"), Fast);
	TestNotNull(TEXT("Director asset exists"), Director);
	if (Basic) { TestTrue(TEXT("Basic enemy grants more XP than a tree"), Basic->ExperienceReward > 3); }
	if (Fast) { TestTrue(TEXT("Fast enemy grants more XP than basic enemy"), Fast->ExperienceReward > (Basic ? Basic->ExperienceReward : 0)); }
	if (Director)
	{
		TestEqual(TEXT("Two enemy families are enabled"), Director->NightEnemies.Num(), 2);
		TestTrue(TEXT("Density is bounded"), Director->MaxAliveEnemies > 0);
	}
	return true;
}

#endif
