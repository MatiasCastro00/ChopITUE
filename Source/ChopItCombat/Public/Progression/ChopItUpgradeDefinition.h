#pragma once

#include "Combat/ChopItCombatStatsComponent.h"
#include "Engine/DataAsset.h"
#include "ChopItUpgradeDefinition.generated.h"

UENUM(BlueprintType)
enum class EChopItUpgradeRarity : uint8
{
	Common,
	Uncommon,
	Rare
};

/** Data-only level-up choice. New stat-only upgrades require no new runtime class. */
UCLASS(BlueprintType)
class CHOPITCOMBAT_API UChopItUpgradeDefinition final : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FName UpgradeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	EChopItUpgradeRarity Rarity = EChopItUpgradeRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "1"))
	int32 MaxStacks = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "0.0"))
	float OfferWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	TArray<FChopItStatModifier> Modifiers;
};
