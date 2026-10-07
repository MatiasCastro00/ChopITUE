#include "Items/ChopItItemComponent.h"
#include "Items/ChopItItemDataAsset.h"
#include "Items/ChopItItemLootSubsystem.h"
#include "Items/ChopItPassiveEffects.h"
#include "Items/ChopItItemEventSubsystem.h"
#include "Combat/ChopItHealthComponent.h"
#include "Combat/ChopItCombatStatsComponent.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/AssetManager.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "TimerManager.h"
#include "ChopItGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItItemStackTest, "ChopIt.Items.StacksAndRequirements", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItItemStackTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("No stacks"), UChopItItemComponent::CalculateStackedValue(10, 0), 0.f);
	TestEqual(TEXT("First stack"), UChopItItemComponent::CalculateStackedValue(10, 1), 10.f);
	TestEqual(TEXT("Four stacks"), UChopItItemComponent::CalculateStackedValue(10, 4), 18.75f);
	TestEqual(TEXT("Large stack count"), UChopItItemComponent::CalculateStackedValue(10, 100000), 20.f);
	TestEqual(TEXT("Decay zero"), UChopItItemComponent::CalculateStackedValue(10, 10, 0), 10.f);
	TestEqual(TEXT("Decay one"), UChopItItemComponent::CalculateStackedValue(10, 10, 1), 100.f);
	auto* Inventory = NewObject<UChopItItemComponent>();
	auto* Base = NewObject<UChopItItemDataAsset>(); Base->ItemId = TEXT("Wormwood");
	auto* Queen = NewObject<UChopItItemDataAsset>(); Queen->ItemId = TEXT("Queen"); Queen->RequiredItemIds.Add(Base->ItemId);
	TestFalse(TEXT("Missing prerequisite"), UChopItItemLootSubsystem::CanItemAppear(Queen, Inventory));
	TestTrue(TEXT("Add"), Inventory->AddItem(Base, 4));
	TestTrue(TEXT("Prerequisite accepted"), UChopItItemLootSubsystem::CanItemAppear(Queen, Inventory));
	TestFalse(TEXT("Reject negative count"), Inventory->AddItem(Base, -1));
	auto* Duplicate = NewObject<UChopItItemDataAsset>(); Duplicate->ItemId = Base->ItemId;
	TestFalse(TEXT("Reject colliding definition"), Inventory->AddItem(Duplicate));
	Inventory->RemoveItem(Base->ItemId, 3);
	TestEqual(TEXT("Remove partial"), Inventory->GetStackCount(Base->ItemId), 1);
	Inventory->ClearItems();
	TestFalse(TEXT("Removed prerequisite"), UChopItItemLootSubsystem::CanItemAppear(Queen, Inventory));
	Queen->RequiredItemIds.Reset();
	Queen->RequiredTags.AddTag(ChopItGameplayTags::State_Cycle_Day);
	TestFalse(TEXT("Missing required tag"), UChopItItemLootSubsystem::CanItemAppear(Queen, Inventory));
	Base->Tags.AddTag(ChopItGameplayTags::State_Cycle_Day);
	Inventory->AddItem(Base);
	TestTrue(TEXT("Owned tag fulfills requirement"), UChopItItemLootSubsystem::CanItemAppear(Queen, Inventory));
	Queen->BlockedTags = Base->Tags;
	TestFalse(TEXT("Blocked tag excludes item"), UChopItItemLootSubsystem::CanItemAppear(Queen, Inventory));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItItemCatalogTest, "ChopIt.Items.AssetCatalog", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItItemCatalogTest::RunTest(const FString& Parameters)
{
	UChopItItemDataAsset* LuckItem = NewObject<UChopItItemDataAsset>();
	LuckItem->SpawnWeight = 1.f;
	for (int32 Tier = 0; Tier < 4; ++Tier)
	{
		LuckItem->Rarity = static_cast<EChopItItemRarity>(Tier);
		TestEqual(FString::Printf(TEXT("Zero luck preserves tier %d weight"), Tier),
			UChopItItemLootSubsystem::CalculateLuckAdjustedWeight(LuckItem, 0.f), 1.0);
		TestEqual(FString::Printf(TEXT("100 percent luck scales tier %d weight"), Tier),
			UChopItItemLootSubsystem::CalculateLuckAdjustedWeight(LuckItem, 100.f), FMath::Pow(2.0, Tier));
	}
	TArray<FPrimaryAssetId> AllItemIds;
	UAssetManager::Get().GetPrimaryAssetIdList(TEXT("ChopItItem"), AllItemIds);
	TestTrue(TEXT("Debug item picker sees all authored stick assets"), AllItemIds.Num() >= 24);
	auto* GameInstance = NewObject<UGameInstance>();
	auto* Loot = NewObject<UChopItItemLootSubsystem>(GameInstance);
	const auto Available = Loot->GetAvailableItems(nullptr, {});
	TestTrue(TEXT("AssetManager discovers four examples"), Available.Num() >= 4);
	const auto Unique = Loot->GetRandomItems(Available.Num() + 2, nullptr, {});
	TestEqual(TEXT("Unique draw stops when pool is empty"), Unique.Num(), Available.Num());
	TSet<FName> Ids;
	for (const auto* Item : Unique) Ids.Add(Item->ItemId);
	TestEqual(TEXT("No duplicate IDs"), Ids.Num(), Unique.Num());
	const auto Repeated = Loot->GetRandomItems(12, nullptr, {}, true);
	TestEqual(TEXT("Draw with replacement"), Repeated.Num(), 12);
	const auto Rare = Loot->GetAvailableItems(nullptr, {EChopItItemRarity::Rare});
	for (const auto* Item : Rare) TestEqual(TEXT("Rarity filter"), Item->Rarity, EChopItItemRarity::Rare);
	TestEqual(TEXT("Negative count"), Loot->GetRandomItems(-1, nullptr, {}).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItItemRuntimeTest, "ChopIt.Items.RuntimeIntegration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItItemRuntimeTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	AActor* Player = World->SpawnActor<AActor>();
	auto AddComponent = [Player]<typename T>() { auto* C = NewObject<T>(Player); Player->AddInstanceComponent(C); C->RegisterComponent(); return C; };
	auto* Health = AddComponent.operator()<UChopItHealthComponent>();
	auto* Stats = AddComponent.operator()<UChopItCombatStatsComponent>();
	auto* Cargo = AddComponent.operator()<UChopItWoodCargoComponent>();
	auto* Inventory = AddComponent.operator()<UChopItItemComponent>();
	World->BeginPlay();
	// Standalone test worlds do not necessarily dispatch actors through a game mode.
	if (!Player->HasActorBegunPlay()) Player->DispatchBeginPlay();
	auto MakeItem = [](FName Id, UClass* EffectClass, float BaseValue)
	{
		auto* Item = NewObject<UChopItItemDataAsset>(); Item->ItemId = Id;
		auto* Effect = NewObject<UChopItItemEffect>(Item, EffectClass); Effect->BaseValue = BaseValue; Item->Effects.Add(Effect);
		return Item;
	};
	auto* Wood = MakeItem(TEXT("Wood"), UChopItWoodYieldEffect::StaticClass(), 0.5f);
	Inventory->AddItem(Wood);
	Cargo->TryAddWood(1); Cargo->TryAddWood(1);
	TestEqual(TEXT("Fractional bonus accumulates"), Cargo->GetCurrentWood(), 3);
	Inventory->AddItem(Wood);
	TestEqual(TEXT("Modifier updates with stacks"), Stats->EvaluateStat(EChopItCombatStat::WoodYield, 1.f), 1.75f);
	Inventory->RemoveItem(Wood->ItemId, 2);
	TestEqual(TEXT("Modifier removed"), Stats->EvaluateStat(EChopItCombatStat::WoodYield, 1.f), 1.f);
	FChopItDamageSpec Damage; Damage.BaseDamage = 50;
	Health->ApplyDamage(Damage, nullptr);
	World->GetTimerManager().Tick(0.f);
	auto* Regen = MakeItem(TEXT("Regen"), UChopItRegenerationEffect::StaticClass(), 2.f);
	Inventory->AddItem(Regen);
	++GFrameCounter;
	World->GetTimerManager().Tick(1.1f);
	TestEqual(TEXT("Timer heals"), Health->GetCurrentHealth(), 52.f);
	Inventory->RemoveItem(Regen->ItemId);
	++GFrameCounter;
	World->GetTimerManager().Tick(1.1f);
	TestEqual(TEXT("Timer removed"), Health->GetCurrentHealth(), 52.f);
	auto* Vampire = MakeItem(TEXT("Vampire"), UChopItLifeStealEffect::StaticClass(), 0.1f);
	auto* Infestation = MakeItem(TEXT("Infestation"), UChopItInfestationEffect::StaticClass(), 0.5f);
	Inventory->AddItem(Vampire); Inventory->AddItem(Infestation);
	AActor* Enemy = World->SpawnActor<AActor>();
	auto* EnemyHealth = NewObject<UChopItHealthComponent>(Enemy);
	Enemy->AddInstanceComponent(EnemyHealth); EnemyHealth->RegisterComponent();
	EnemyHealth->TargetKind = EChopItDamageTargetKind::Enemy;
	int32 Deaths = 0; int32 Executions = 0; float Transferrable = 0.f;
	EnemyHealth->OnDeath.AddLambda([&](AActor*, AActor*) { ++Deaths; });
	auto* Events = World->GetSubsystem<UChopItItemEventSubsystem>();
	const auto Handle = Events->OnEvent.AddLambda([&](const FChopItItemEventContext& Event)
	{
		if (Event.Event == EChopItItemEvent::InfestationExecuted) { ++Executions; Transferrable = Event.Amount; }
	});
	Damage.BaseDamage = 40;
	EnemyHealth->ApplyDamage(Damage, Player);
	TestEqual(TEXT("Vampire heals actual damage"), Health->GetCurrentHealth(), 56.f);
	TestEqual(TEXT("Infestation does not immediately subtract health"), EnemyHealth->GetCurrentHealth(), 60.f);
	EnemyHealth->ApplyDamage(Damage, Player);
	TestEqual(TEXT("Execution goes through existing death once"), Deaths, 1);
	TestEqual(TEXT("One execution event"), Executions, 1);
	TestEqual(TEXT("Accumulated infestation available for synergy"), Transferrable, 20.f);
	TestEqual(TEXT("Execution excluded from lifesteal"), Health->GetCurrentHealth(), 60.f);
	TestEqual(TEXT("Asset template contains no stacks"), Infestation->Effects[0]->Stacks, 0);
	TestNull(TEXT("Asset template contains no owner"), Infestation->Effects[0]->Inventory.Get());
	Events->OnEvent.Remove(Handle);
	Inventory->ClearItems();
	TestEqual(TEXT("All item modifiers cleaned"), Stats->GetModifierCount(), 0);
	World->DestroyWorld(false);
	return true;
}
#endif
