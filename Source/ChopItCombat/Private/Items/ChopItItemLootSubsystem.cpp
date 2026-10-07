#include "Items/ChopItItemLootSubsystem.h"
#include "Items/ChopItItemDataAsset.h"
#include "Items/ChopItItemComponent.h"
#include "Engine/AssetManager.h"

void UChopItItemLootSubsystem::RefreshCatalog()
{
	Catalog.Reset();
	TArray<FPrimaryAssetId> Ids;
	UAssetManager& Manager = UAssetManager::Get();
	Manager.GetPrimaryAssetIdList(TEXT("ChopItItem"), Ids);
	for (const auto& Id : Ids)
		if (auto* Item = Cast<UChopItItemDataAsset>(Manager.GetPrimaryAssetPath(Id).TryLoad())) Catalog.Add(Item);
	bLoaded = true;
}
bool UChopItItemLootSubsystem::CanItemAppear(const UChopItItemDataAsset* Item, const UChopItItemComponent* Inventory)
{
	if (!IsValid(Item) || Item->ItemId.IsNone() || !FMath::IsFinite(Item->SpawnWeight) || Item->SpawnWeight <= 0.f) return false;
	for (FName Id : Item->RequiredItemIds) if (!Inventory || Inventory->GetStackCount(Id) <= 0) return false;
	const FGameplayTagContainer Tags = Inventory ? Inventory->GetOwnedTags() : FGameplayTagContainer();
	return Tags.HasAll(Item->RequiredTags) && !Tags.HasAny(Item->BlockedTags);
}
TArray<UChopItItemDataAsset*> UChopItItemLootSubsystem::GetAvailableItems(const UChopItItemComponent* Inventory, const TArray<EChopItItemRarity>& Rarities)
{
	if (!bLoaded) RefreshCatalog();
	TArray<UChopItItemDataAsset*> Result;
	for (UChopItItemDataAsset* Item : Catalog)
		if (CanItemAppear(Item, Inventory) && (Rarities.IsEmpty() || Rarities.Contains(Item->Rarity))) Result.Add(Item);
	return Result;
}
UChopItItemDataAsset* UChopItItemLootSubsystem::GetRandomItem(const UChopItItemComponent* Inventory, const TArray<EChopItItemRarity>& Rarities)
{
	const auto Result = GetRandomItems(1, Inventory, Rarities);
	return Result.IsEmpty() ? nullptr : Result[0];
}
double UChopItItemLootSubsystem::CalculateLuckAdjustedWeight(const UChopItItemDataAsset* Item, float LuckPercent)
{
	if (!IsValid(Item) || !FMath::IsFinite(Item->SpawnWeight) || Item->SpawnWeight <= 0.f) return 0.;
	const double SafeLuck = FMath::IsFinite(LuckPercent) ? FMath::Clamp(static_cast<double>(LuckPercent), 0., 1000.) : 0.;
	const int32 Tier = static_cast<int32>(Item->Rarity);
	return static_cast<double>(Item->SpawnWeight) * FMath::Pow(1. + SafeLuck / 100., Tier);
}
UChopItItemDataAsset* UChopItItemLootSubsystem::GetRandomItemWithLuck(const UChopItItemComponent* Inventory, float LuckPercent)
{
	const TArray<UChopItItemDataAsset*> Pool = GetAvailableItems(Inventory, {});
	if (Pool.IsEmpty()) return nullptr;
	double Total = 0.;
	for (const UChopItItemDataAsset* Item : Pool) Total += CalculateLuckAdjustedWeight(Item, LuckPercent);
	if (Total <= 0.) return nullptr;
	double Roll = FMath::FRand() * Total;
	for (UChopItItemDataAsset* Item : Pool)
	{
		Roll -= CalculateLuckAdjustedWeight(Item, LuckPercent);
		if (Roll < 0.) return Item;
	}
	return Pool.Last();
}
TArray<UChopItItemDataAsset*> UChopItItemLootSubsystem::GetRandomItems(int32 Count, const UChopItItemComponent* Inventory, const TArray<EChopItItemRarity>& Rarities, bool bAllowDuplicates)
{
	auto Pool = GetAvailableItems(Inventory, Rarities);
	TArray<UChopItItemDataAsset*> Result;
	while (Result.Num() < Count && !Pool.IsEmpty())
	{
		double Total = 0.; for (auto* Item : Pool) Total += Item->SpawnWeight;
		double Roll = FMath::FRand() * Total;
		int32 Selected = Pool.Num() - 1;
		for (int32 Index = 0; Index < Pool.Num(); ++Index)
		{
			Roll -= Pool[Index]->SpawnWeight;
			if (Roll < 0.) { Selected = Index; break; }
		}
		Result.Add(Pool[Selected]);
		if (!bAllowDuplicates) Pool.RemoveAt(Selected);
	}
	return Result;
}
