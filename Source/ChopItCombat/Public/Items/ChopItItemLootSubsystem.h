#pragma once
#include "Items/ChopItItemTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ChopItItemLootSubsystem.generated.h"

class UChopItItemComponent;

/** Source-agnostic catalog; load once at a suitable loading screen using RefreshCatalog. */
UCLASS()
class CHOPITCOMBAT_API UChopItItemLootSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="ChopIt|Items|Loot") void RefreshCatalog();
	UFUNCTION(BlueprintPure, Category="ChopIt|Items|Loot") static bool CanItemAppear(const UChopItItemDataAsset* Item, const UChopItItemComponent* Inventory);
	/** Empty rarities means all rarities. */
	UFUNCTION(BlueprintCallable, Category="ChopIt|Items|Loot") TArray<UChopItItemDataAsset*> GetAvailableItems(const UChopItItemComponent* Inventory, const TArray<EChopItItemRarity>& Rarities);
	UFUNCTION(BlueprintCallable, Category="ChopIt|Items|Loot") UChopItItemDataAsset* GetRandomItem(const UChopItItemComponent* Inventory, const TArray<EChopItItemRarity>& Rarities);
	/** Luck is a non-negative percentage; each rarity tier gains another luck multiplier. */
	UFUNCTION(BlueprintCallable, Category="ChopIt|Items|Loot") UChopItItemDataAsset* GetRandomItemWithLuck(const UChopItItemComponent* Inventory, float LuckPercent);
	static double CalculateLuckAdjustedWeight(const UChopItItemDataAsset* Item, float LuckPercent);
	UFUNCTION(BlueprintCallable, Category="ChopIt|Items|Loot") TArray<UChopItItemDataAsset*> GetRandomItems(int32 Count, const UChopItItemComponent* Inventory, const TArray<EChopItItemRarity>& Rarities, bool bAllowDuplicates = false);
private:
	UPROPERTY(Transient) TArray<TObjectPtr<UChopItItemDataAsset>> Catalog;
	bool bLoaded = false;
};
