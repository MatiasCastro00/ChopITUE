#include "Economy/ChopItEconomyComponent.h"
#include "Framework/ChopItPlayerState.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Shop/ChopItShopComponent.h"
#include "Weapons/ChopItWeaponDefinition.h"
#include "Weapons/ChopItWeaponLoadoutComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase7LoadoutRulesTest,
	"ChopIt.Phase7.LoadoutRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase7LoadoutRulesTest::RunTest(const FString& Parameters)
{
	UChopItWeaponLoadoutComponent* Loadout = NewObject<UChopItWeaponLoadoutComponent>();
	UChopItWeaponDefinition* SharedWeapon = NewObject<UChopItWeaponDefinition>();
	SharedWeapon->WeaponId = TEXT("SharedTestWeapon");
	SharedWeapon->bUsesLoadoutSlot = true;
	UChopItWeaponDefinition* ExclusiveWeapon = NewObject<UChopItWeaponDefinition>();
	ExclusiveWeapon->WeaponId = TEXT("ExclusiveTestWeapon");
	ExclusiveWeapon->bExclusiveToStartingCharacter = true;
	ExclusiveWeapon->bUsesLoadoutSlot = false;
	TestTrue(TEXT("Shared weapon is valid for an empty loadout"), Loadout->CanEquipWeapon(SharedWeapon));
	TestFalse(TEXT("Starting exclusive weapon cannot be bought"), Loadout->CanEquipWeapon(ExclusiveWeapon));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase7EconomyTransactionTest,
	"ChopIt.Phase7.EconomyTransaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase7EconomyTransactionTest::RunTest(const FString& Parameters)
{
	UChopItEconomyComponent* Economy = NewObject<UChopItEconomyComponent>();
	const FGuid IncomeId = FGuid::NewGuid();
	const FGuid PurchaseId = FGuid::NewGuid();
	TestTrue(TEXT("Sale creates spendable balance"), Economy->ApplyTransaction(IncomeId, TEXT("Sale"), 40));
	TestTrue(TEXT("Purchase debit is recorded once"), Economy->ApplyTransaction(PurchaseId, TEXT("WeaponPurchase"), -20));
	TestEqual(TEXT("Purchase has exact remaining balance"), Economy->GetBalance(), int64(20));
	TestFalse(TEXT("Duplicate purchase transaction is idempotently rejected"), Economy->ApplyTransaction(PurchaseId, TEXT("WeaponPurchase"), -20));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase7AssetsTest,
	"ChopIt.Phase7.Assets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase7AssetsTest::RunTest(const FString& Parameters)
{
	const UChopItWeaponDefinition* HandSaw = LoadObject<UChopItWeaponDefinition>(nullptr, TEXT("/Game/ChopIt/Combat/Weapons/DA_Weapon_HandSaw.DA_Weapon_HandSaw"));
	const UChopItWeaponDefinition* SawHalo = LoadObject<UChopItWeaponDefinition>(nullptr, TEXT("/Game/ChopIt/Combat/Weapons/DA_Weapon_SawHalo.DA_Weapon_SawHalo"));
	TestNotNull(TEXT("Hand saw asset exists"), HandSaw);
	TestNotNull(TEXT("Circular saw asset exists"), SawHalo);
	if (HandSaw) TestEqual(TEXT("Hand saw name is English"), HandSaw->DisplayName.ToString(), FString(TEXT("Hand Chainsaw")));
	if (SawHalo) TestEqual(TEXT("Saw halo name is English"), SawHalo->DisplayName.ToString(), FString(TEXT("Saw Halo")));
	TestTrue(TEXT("Shop map exists"), FPackageName::DoesPackageExist(TEXT("/Game/ChopIt/World/Maps/L_Test_Shop")));
	const AChopItPlayerState* State = GetDefault<AChopItPlayerState>();
	TestNotNull(TEXT("PlayerState owns shop session"), State->GetShopComponent());
	return true;
}

#endif
