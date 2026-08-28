#include "Combat/ChopItCombatStatsComponent.h"
#include "Combat/ChopItDamageTypes.h"
#include "Combat/ChopItHealthComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Harvest/ChopItTree.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Targets/ChopItCombatDummy.h"
#include "Targeting/ChopItTargetingSubsystem.h"
#include "Weapons/ChopItWeaponDefinition.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase2DamageFormulaTest,
	"ChopIt.Phase2.DamageFormula",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase2DamageFormulaTest::RunTest(const FString& Parameters)
{
	FChopItDamageSpec Spec;
	Spec.BaseDamage = 25.0f;
	TestEqual(TEXT("Base damage is deterministic"), Spec.CalculateFinalDamage(), 25.0f);

	Spec.DamageMultiplier = 1.5f;
	TestEqual(TEXT("Damage multiplier is applied"), Spec.CalculateFinalDamage(), 37.5f);

	Spec.bCritical = true;
	Spec.CriticalMultiplier = 2.0f;
	TestEqual(TEXT("Critical multiplier is applied last"), Spec.CalculateFinalDamage(), 75.0f);

	Spec.BaseDamage = -100.0f;
	TestEqual(TEXT("Negative damage is clamped"), Spec.CalculateFinalDamage(), 0.0f);

	const FVector Origin = FVector::ZeroVector;
	const FVector Forward = FVector::ForwardVector;
	TestTrue(TEXT("Target directly ahead is inside axe arc"), UChopItTargetingSubsystem::IsLocationInsideArc(Origin, Forward, FVector(400, 0, 0), 500.0f, 55.0f));
	TestFalse(TEXT("Target behind is outside axe arc"), UChopItTargetingSubsystem::IsLocationInsideArc(Origin, Forward, FVector(-100, 0, 0), 500.0f, 55.0f));
	TestFalse(TEXT("Target to the side is outside axe arc"), UChopItTargetingSubsystem::IsLocationInsideArc(Origin, Forward, FVector(0, 100, 0), 500.0f, 55.0f));
	TestFalse(TEXT("Target beyond range is outside axe arc"), UChopItTargetingSubsystem::IsLocationInsideArc(Origin, Forward, FVector(501, 0, 0), 500.0f, 55.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase2ModifierStackTest,
	"ChopIt.Phase2.ModifierStack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase2ModifierStackTest::RunTest(const FString& Parameters)
{
	UChopItCombatStatsComponent* Stats = NewObject<UChopItCombatStatsComponent>();
	TArray<FGuid> Handles;
	Handles.Reserve(1000);
	FChopItStatModifier Modifier;
	Modifier.Stat = EChopItCombatStat::Damage;
	Modifier.Operation = EChopItModifierOperation::Add;
	Modifier.Magnitude = 0.01f;
	for (int32 Index = 0; Index < 1000; ++Index)
	{
		Handles.Add(Stats->AddModifier(Modifier));
	}

	TestEqual(TEXT("All modifier handles are retained"), Stats->GetModifierCount(), 1000);
	TestTrue(TEXT("One thousand modifiers produce the expected value"), FMath::IsNearlyEqual(Stats->EvaluateStat(EChopItCombatStat::Damage, 20.0f), 30.0f, 0.01f));
	for (const FGuid& Handle : Handles)
	{
		TestTrue(TEXT("Each modifier can be removed exactly once"), Stats->RemoveModifier(Handle));
	}
	TestEqual(TEXT("Modifier storage returns to empty"), Stats->GetModifierCount(), 0);
	TestEqual(TEXT("Base value is restored"), Stats->EvaluateStat(EChopItCombatStat::Damage, 20.0f), 20.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase2HealthLifecycleTest,
	"ChopIt.Phase2.HealthLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase2HealthLifecycleTest::RunTest(const FString& Parameters)
{
	UChopItHealthComponent* Health = NewObject<UChopItHealthComponent>();
	int32 DeathCount = 0;
	Health->OnDeath.AddLambda([&DeathCount](AActor*, AActor*) { ++DeathCount; });

	FChopItDamageSpec Damage;
	Damage.BaseDamage = 30.0f;
	TestEqual(TEXT("First hit applies full damage"), Health->ApplyDamage(Damage, nullptr), 30.0f);
	TestEqual(TEXT("Health is reduced"), Health->GetCurrentHealth(), 70.0f);

	Damage.BaseDamage = 500.0f;
	TestEqual(TEXT("Lethal hit only applies remaining health"), Health->ApplyDamage(Damage, nullptr), 70.0f);
	TestFalse(TEXT("Target is dead"), Health->IsAlive());
	TestEqual(TEXT("Death fires once"), DeathCount, 1);
	TestEqual(TEXT("Damage after death is ignored"), Health->ApplyDamage(Damage, nullptr), 0.0f);
	TestEqual(TEXT("Death still fired once"), DeathCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase2AssetsAndMapTest,
	"ChopIt.Phase2.AssetsAndMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase2AssetsAndMapTest::RunTest(const FString& Parameters)
{
	const UChopItWeaponDefinition* Weapon = LoadObject<UChopItWeaponDefinition>(
		nullptr,
		TEXT("/Game/ChopIt/Combat/Weapons/DA_Weapon_BasicAxe.DA_Weapon_BasicAxe"));
	TestNotNull(TEXT("Basic axe data asset exists"), Weapon);
	if (!Weapon)
	{
		return false;
	}
	TestTrue(TEXT("Weapon has positive damage"), Weapon->Damage > 0.0f);
	TestTrue(TEXT("Weapon has positive cadence"), Weapon->AttackInterval > 0.0f);
	TestTrue(TEXT("Weapon has positive range"), Weapon->Range > 0.0f);
	TestTrue(TEXT("Weapon has a frontal arc"), Weapon->ArcHalfAngleDegrees > 0.0f && Weapon->ArcHalfAngleDegrees < 90.0f);
	TestEqual(TEXT("Basic axe cleave target cap"), Weapon->MaxTargets, 3);
	TestFalse(TEXT("Criticals are disabled in the baseline"), Weapon->CriticalChance > 0.0f);
	TestEqual(TEXT("Weapon is registered under the expected primary type"), Weapon->GetPrimaryAssetId().PrimaryAssetType, FPrimaryAssetType(TEXT("ChopItWeapon")));

	const FString CombatMapPackage = TEXT("/Game/ChopIt/World/Maps/L_Test_Combat");
	TestTrue(TEXT("Combat test map package exists"), FPackageName::DoesPackageExist(CombatMapPackage));
	UPackage* Package = LoadPackage(nullptr, *CombatMapPackage, LOAD_None);
	TestNotNull(TEXT("Combat map package loads"), Package);
	if (!Package)
	{
		return false;
	}

	UWorld* World = FindObject<UWorld>(Package, TEXT("L_Test_Combat"));
	TestNotNull(TEXT("Combat map world exists"), World);
	if (!World)
	{
		return false;
	}

	int32 DummyCount = 0;
	int32 TreeCount = 0;
	for (AActor* Actor : World->PersistentLevel->Actors)
	{
		DummyCount += IsValid(Actor) && Actor->IsA<AChopItCombatDummy>() ? 1 : 0;
		TreeCount += IsValid(Actor) && Actor->IsA<AChopItTree>() ? 1 : 0;
	}
	TestEqual(TEXT("Combat map has four deterministic dummies"), DummyCount, 4);
	TestEqual(TEXT("Combat map has twenty damageable trees"), TreeCount, 20);
	return true;
}

#endif
