#pragma once
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Items/ChopItItemTypes.h"
#include "ChopItItemDataAsset.generated.h"

class UChopItItemEffect;
class UTexture2D;

/** Effect templates contain configuration only. Inventory duplicates them for each owner. */
UCLASS(BlueprintType)
class CHOPITCOMBAT_API UChopItItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") FName ItemId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(MultiLine=true)) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(AssetBundles="UI")) TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") EChopItItemRarity Rarity = EChopItItemRarity::Common;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") FGameplayTagContainer Tags;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stacking", meta=(ClampMin="0", ClampMax="1")) float StackDecay = 0.5f;
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category="Effects") TArray<TObjectPtr<UChopItItemEffect>> Effects;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot") TArray<FName> RequiredItemIds;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot") FGameplayTagContainer RequiredTags;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot") FGameplayTagContainer BlockedTags;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot", meta=(ClampMin="0")) float SpawnWeight = 1.f;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("ChopItItem"), ItemId); }
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
