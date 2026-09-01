#include "ChopItCollision.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Economy/ChopItCabinHub.h"
#include "Economy/ChopItDayDefinition.h"
#include "Economy/ChopItDeliveryZone.h"
#include "Economy/ChopItEconomyComponent.h"
#include "Economy/ChopItQuotaComponent.h"
#include "Economy/ChopItQuotaMachine.h"
#include "Economy/ChopItSellZone.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Framework/ChopItGameState.h"
#include "Framework/ChopItPlayerState.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase4QuotaTransactionsTest,
	"ChopIt.Phase4.QuotaTransactions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase4QuotaTransactionsTest::RunTest(const FString& Parameters)
{
	UChopItQuotaComponent* Quota = NewObject<UChopItQuotaComponent>();
	Quota->InitializeQuota(3);
	TestEqual(TEXT("Quota target is exact"), Quota->GetTarget(), 3);
	TestEqual(TEXT("Quota starts empty"), Quota->GetProgress(), 0);
	TestFalse(TEXT("Quota has no Tick"), Quota->PrimaryComponentTick.bCanEverTick);

	const FGuid TransactionId = FGuid::NewGuid();
	const FChopItQuotaTransferResult Result = Quota->TryContributeWood(TransactionId, 5);
	TestEqual(TEXT("Quota accepts only its remaining target"), Result.Accepted, 3);
	TestEqual(TEXT("Quota preserves excess"), Result.Remainder, 2);
	TestTrue(TEXT("Transfer completes quota once"), Result.bCompletedNow);
	TestTrue(TEXT("Quota is complete"), Quota->IsComplete());

	const FChopItQuotaTransferResult Duplicate = Quota->TryContributeWood(TransactionId, 5);
	TestTrue(TEXT("Repeated transaction is identified"), Duplicate.bDuplicate);
	TestEqual(TEXT("Duplicate accepts nothing"), Duplicate.Accepted, 0);
	TestEqual(TEXT("Duplicate does not change progress"), Quota->GetProgress(), 3);
	TestEqual(TEXT("Only one transaction ID is retained"), Quota->GetProcessedTransactionCount(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase4EconomyLedgerTest,
	"ChopIt.Phase4.EconomyLedger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase4EconomyLedgerTest::RunTest(const FString& Parameters)
{
	UChopItEconomyComponent* Economy = NewObject<UChopItEconomyComponent>();
	const FGuid SaleId = FGuid::NewGuid();
	TestTrue(TEXT("Sale credits balance"), Economy->ApplyTransaction(SaleId, TEXT("WoodSale"), 12));
	TestEqual(TEXT("Balance is int64 and exact"), Economy->GetBalance(), int64(12));
	TestFalse(TEXT("Duplicate sale is rejected"), Economy->ApplyTransaction(SaleId, TEXT("WoodSale"), 12));
	TestEqual(TEXT("Duplicate does not credit twice"), Economy->GetBalance(), int64(12));
	TestEqual(TEXT("Ledger has one sale"), Economy->GetLedger().Num(), 1);
	TestFalse(TEXT("Transaction cannot overdraw"), Economy->ApplyTransaction(FGuid::NewGuid(), TEXT("Purchase"), -13));
	TestTrue(TEXT("Valid debit succeeds"), Economy->ApplyTransaction(FGuid::NewGuid(), TEXT("Purchase"), -4));
	TestEqual(TEXT("Debit updates exact balance"), Economy->GetBalance(), int64(8));

	UChopItEconomyComponent* Extreme = NewObject<UChopItEconomyComponent>();
	TestTrue(TEXT("Maximum balance can be represented"), Extreme->ApplyTransaction(FGuid::NewGuid(), TEXT("Stress"), MAX_int64));
	TestFalse(TEXT("Overflow is rejected"), Extreme->ApplyTransaction(FGuid::NewGuid(), TEXT("Overflow"), 1));
	TestEqual(TEXT("Rejected overflow preserves balance"), Extreme->GetBalance(), MAX_int64);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase4PriorityAndConservationTest,
	"ChopIt.Phase4.PriorityAndConservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase4PriorityAndConservationTest::RunTest(const FString& Parameters)
{
	UChopItQuotaComponent* Quota = NewObject<UChopItQuotaComponent>();
	UChopItWoodCargoComponent* Cargo = NewObject<UChopItWoodCargoComponent>();
	UChopItEconomyComponent* Economy = NewObject<UChopItEconomyComponent>();
	Quota->InitializeQuota(3);
	Cargo->TryAddWood(5);
	TestFalse(TEXT("Sale is blocked before quota"), AChopItSellZone::CanSell(Quota, Cargo, Economy));

	const FChopItQuotaTransferResult Delivery = Quota->TryContributeWood(FGuid::NewGuid(), Cargo->GetCurrentWood());
	Cargo->TryRemoveWood(Delivery.Accepted);
	TestEqual(TEXT("Three units reach quota"), Quota->GetProgress(), 3);
	TestEqual(TEXT("Two excess units remain in cargo"), Cargo->GetCurrentWood(), 2);
	TestEqual(TEXT("Quota plus cargo conserves all wood"), Quota->GetProgress() + Cargo->GetCurrentWood(), 5);
	TestTrue(TEXT("Sale unlocks only after quota"), AChopItSellZone::CanSell(Quota, Cargo, Economy));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase4AssetsAndMapTest,
	"ChopIt.Phase4.AssetsAndMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase4AssetsAndMapTest::RunTest(const FString& Parameters)
{
	const AChopItCabinHub* CabinDefaults = GetDefault<AChopItCabinHub>();
	const UStaticMeshComponent* CabinVisual = CabinDefaults ? CabinDefaults->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	TestNotNull(TEXT("Cabin has a camera-occludable visual"), CabinVisual);
	if (CabinVisual)
	{
		TestEqual(TEXT("Cabin never pushes the camera"), CabinVisual->GetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid), ECR_Ignore);
	}
	const AChopItQuotaMachine* MachineDefaults = GetDefault<AChopItQuotaMachine>();
	const UStaticMeshComponent* MachineVisual = MachineDefaults ? MachineDefaults->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	TestNotNull(TEXT("Quota lever has a camera-occludable visual"), MachineVisual);
	if (MachineVisual)
	{
		TestEqual(TEXT("Quota lever never pushes the camera"), MachineVisual->GetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid), ECR_Ignore);
	}

	const UChopItDayDefinition* Day = LoadObject<UChopItDayDefinition>(
		nullptr, TEXT("/Game/ChopIt/Economy/Days/DA_Day_01.DA_Day_01"));
	TestNotNull(TEXT("Day one definition exists"), Day);
	if (Day)
	{
		TestEqual(TEXT("Day one quota"), Day->WoodQuota, 3);
		TestEqual(TEXT("Day one wood value"), Day->MoneyPerWood, int64(4));
		TestEqual(TEXT("Day definition primary type"), Day->GetPrimaryAssetId().PrimaryAssetType, FPrimaryAssetType(TEXT("ChopItDay")));
	}

	TestNotNull(TEXT("BP_CabinHub exists"), LoadClass<AChopItCabinHub>(nullptr, TEXT("/Game/ChopIt/World/Economy/BP_CabinHub.BP_CabinHub_C")));
	TestNotNull(TEXT("BP_QuotaMachine exists"), LoadClass<AChopItQuotaMachine>(nullptr, TEXT("/Game/ChopIt/World/Economy/BP_QuotaMachine.BP_QuotaMachine_C")));
	TestNotNull(TEXT("BP_DeliveryZone exists"), LoadClass<AChopItDeliveryZone>(nullptr, TEXT("/Game/ChopIt/World/Economy/BP_DeliveryZone.BP_DeliveryZone_C")));
	TestNotNull(TEXT("BP_SellZone exists"), LoadClass<AChopItSellZone>(nullptr, TEXT("/Game/ChopIt/World/Economy/BP_SellZone.BP_SellZone_C")));

	const FString MapPackageName = TEXT("/Game/ChopIt/World/Maps/L_Test_Economy");
	TestTrue(TEXT("Economy map exists"), FPackageName::DoesPackageExist(MapPackageName));
	UPackage* Package = LoadPackage(nullptr, *MapPackageName, LOAD_None);
	UWorld* World = Package ? FindObject<UWorld>(Package, TEXT("L_Test_Economy")) : nullptr;
	TestNotNull(TEXT("Economy map loads"), World);
	if (!World)
	{
		return false;
	}

	int32 CabinCount = 0;
	int32 MachineCount = 0;
	int32 DeliveryCount = 0;
	int32 SellCount = 0;
	for (AActor* Actor : World->PersistentLevel->Actors)
	{
		CabinCount += IsValid(Actor) && Actor->IsA<AChopItCabinHub>() ? 1 : 0;
		MachineCount += IsValid(Actor) && Actor->IsA<AChopItQuotaMachine>() ? 1 : 0;
		DeliveryCount += IsValid(Actor) && Actor->IsA<AChopItDeliveryZone>() ? 1 : 0;
		SellCount += IsValid(Actor) && Actor->IsA<AChopItSellZone>() ? 1 : 0;
	}
	TestEqual(TEXT("One cabin hub"), CabinCount, 1);
	TestEqual(TEXT("One quota machine"), MachineCount, 1);
	TestEqual(TEXT("One quota delivery zone"), DeliveryCount, 1);
	TestEqual(TEXT("One truck sell zone"), SellCount, 1);

	const AChopItDeliveryZone* DeliveryCDO = GetDefault<AChopItDeliveryZone>();
	const AChopItSellZone* SellCDO = GetDefault<AChopItSellZone>();
	TestFalse(TEXT("Delivery zone has no Tick"), DeliveryCDO->PrimaryActorTick.bCanEverTick);
	TestFalse(TEXT("Sell zone has no Tick"), SellCDO->PrimaryActorTick.bCanEverTick);
	TestEqual(TEXT("Delivery is query-only"), DeliveryCDO->GetDeliverySphere()->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
	TestEqual(TEXT("Sell is query-only"), SellCDO->GetSellSphere()->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);

	const AChopItGameState* GameState = GetDefault<AChopItGameState>();
	const AChopItPlayerState* PlayerState = GetDefault<AChopItPlayerState>();
	TestNotNull(TEXT("GameState owns quota"), GameState->GetQuotaComponent());
	TestNotNull(TEXT("PlayerState owns economy"), PlayerState->GetEconomyComponent());
	return true;
}

#endif
