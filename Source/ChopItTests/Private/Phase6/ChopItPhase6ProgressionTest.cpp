#include "Combat/ChopItCombatStatsComponent.h"
#include "Curves/CurveFloat.h"
#include "Framework/ChopItPlayerState.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Progression/ChopItExperienceComponent.h"
#include "Progression/ChopItUpgradeDefinition.h"
#include "Progression/ChopItUpgradeOfferComponent.h"
#include "Framework/ChopItGameMode.h"
#include "UI/ChopItHUD.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase6ExperienceQueueTest,
	"ChopIt.Phase6.ExperienceQueue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase6ExperienceQueueTest::RunTest(const FString& Parameters)
{
	UCurveFloat* Curve = NewObject<UCurveFloat>();
	Curve->FloatCurve.AddKey(1.0f, 10.0f);
	Curve->FloatCurve.AddKey(2.0f, 20.0f);
	Curve->FloatCurve.AddKey(3.0f, 30.0f);
	UChopItExperienceComponent* Experience = NewObject<UChopItExperienceComponent>();
	Experience->SetExperienceCurve(Curve);
	Experience->AddExperience(35);
	TestEqual(TEXT("Crossing two thresholds reaches level three"), Experience->GetLevel(), 3);
	TestEqual(TEXT("No experience is lost"), Experience->GetCurrentExperience(), 5);
	TestEqual(TEXT("Both choices remain queued"), Experience->GetPendingLevelUps(), 2);
	TestTrue(TEXT("First pending level consumes once"), Experience->ConsumePendingLevelUp());
	TestTrue(TEXT("Second pending level consumes once"), Experience->ConsumePendingLevelUp());
	TestFalse(TEXT("Empty queue cannot be consumed twice"), Experience->ConsumePendingLevelUp());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase6ModifierHandlesTest,
	"ChopIt.Phase6.ModifierHandles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase6ModifierHandlesTest::RunTest(const FString& Parameters)
{
	UChopItCombatStatsComponent* Stats = NewObject<UChopItCombatStatsComponent>();
	UChopItUpgradeDefinition* Upgrade = NewObject<UChopItUpgradeDefinition>();
	Upgrade->UpgradeId = TEXT("TestDamage");
	Upgrade->MaxStacks = 2;
	FChopItStatModifier Modifier;
	Modifier.Stat = EChopItCombatStat::Damage;
	Modifier.Operation = EChopItModifierOperation::Multiply;
	Modifier.Magnitude = 1.2f;
	Upgrade->Modifiers.Add(Modifier);
	UChopItUpgradeOfferComponent* Offers = NewObject<UChopItUpgradeOfferComponent>();
	Offers->SetCatalogForAutomation({ Upgrade });
	TestTrue(TEXT("First stack applies"), Offers->ApplyUpgrade(Upgrade, Stats));
	TestTrue(TEXT("Second stack applies"), Offers->ApplyUpgrade(Upgrade, Stats));
	TestFalse(TEXT("Configured stack cap is enforced"), Offers->ApplyUpgrade(Upgrade, Stats));
	TestEqual(TEXT("Multipliers compose from base every time"), Stats->EvaluateStat(EChopItCombatStat::Damage, 100.0f), 144.0f);
	TestTrue(TEXT("One source can be removed by handles"), Offers->RemoveLastStack(TEXT("TestDamage"), Stats));
	TestEqual(TEXT("Removing restores the remaining source exactly"), Stats->EvaluateStat(EChopItCombatStat::Damage, 100.0f), 120.0f);
	TestTrue(TEXT("Final source can be removed"), Offers->RemoveLastStack(TEXT("TestDamage"), Stats));
	TestEqual(TEXT("Base value is recovered"), Stats->EvaluateStat(EChopItCombatStat::Damage, 100.0f), 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase6DeterministicOffersTest,
	"ChopIt.Phase6.DeterministicOffers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase6DeterministicOffersTest::RunTest(const FString& Parameters)
{
	TArray<UChopItUpgradeDefinition*> Catalog;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		UChopItUpgradeDefinition* Upgrade = NewObject<UChopItUpgradeDefinition>();
		Upgrade->UpgradeId = FName(*FString::Printf(TEXT("Upgrade_%d"), Index));
		Upgrade->MaxStacks = 1;
		Upgrade->OfferWeight = static_cast<float>(Index + 1);
		Catalog.Add(Upgrade);
	}
	UChopItUpgradeOfferComponent* First = NewObject<UChopItUpgradeOfferComponent>();
	UChopItUpgradeOfferComponent* Second = NewObject<UChopItUpgradeOfferComponent>();
	First->SetCatalogForAutomation(Catalog);
	Second->SetCatalogForAutomation(Catalog);
	First->GenerateOffersForAutomation(2, 4242);
	Second->GenerateOffersForAutomation(2, 4242);
	TestEqual(TEXT("Offer count"), First->GetActiveOffers().Num(), 3);
	TSet<FName> Unique;
	for (int32 Index = 0; Index < First->GetActiveOffers().Num(); ++Index)
	{
		const FName FirstId = First->GetActiveOffers()[Index]->UpgradeId;
		Unique.Add(FirstId);
		TestEqual(TEXT("Same seed returns same ordered offer"), FirstId, Second->GetActiveOffers()[Index]->UpgradeId);
	}
	TestEqual(TEXT("Offers are selected without replacement"), Unique.Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase6AssetsTest,
	"ChopIt.Phase6.Assets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase6AssetsTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("XP curve exists"), LoadObject<UCurveFloat>(nullptr, TEXT("/Game/ChopIt/Progression/Curves/Curve_XP_Levels.Curve_XP_Levels")));
	static const TCHAR* Names[] = { TEXT("Filo"), TEXT("Ritmo"), TEXT("Alcance"), TEXT("Critico"), TEXT("Botas"), TEXT("Furia"), TEXT("Precision"), TEXT("Gigante") };
	for (const TCHAR* Name : Names)
	{
		const FString Path = FString::Printf(TEXT("/Game/ChopIt/Progression/Upgrades/DA_Upgrade_%s.DA_Upgrade_%s"), Name, Name);
		TestNotNull(*FString::Printf(TEXT("Upgrade %s exists"), Name), LoadObject<UChopItUpgradeDefinition>(nullptr, *Path));
	}
	TestTrue(TEXT("Progression test map exists"), FPackageName::DoesPackageExist(TEXT("/Game/ChopIt/World/Maps/L_Test_Progression")));
	const AChopItPlayerState* State = GetDefault<AChopItPlayerState>();
	TestNotNull(TEXT("PlayerState owns experience"), State->GetExperienceComponent());
	TestNotNull(TEXT("PlayerState owns offers"), State->GetUpgradeOfferComponent());
	TestTrue(TEXT("GameMode uses a screen-space HUD"), GetDefault<AChopItGameMode>()->HUDClass == AChopItHUD::StaticClass());
	return true;
}

#endif
