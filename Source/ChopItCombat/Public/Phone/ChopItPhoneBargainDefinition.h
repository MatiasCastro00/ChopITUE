#pragma once

#include "Combat/ChopItCombatStatsComponent.h"
#include "Engine/DataAsset.h"
#include "ChopItPhoneBargainDefinition.generated.h"

/** A phone offer with an explicit benefit and a price that both affect gameplay. */
UCLASS(BlueprintType)
class CHOPITCOMBAT_API UChopItPhoneBargainDefinition final : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phone Bargain") FName BargainId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phone Bargain") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phone Bargain", meta=(MultiLine=true)) FText BenefitText;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phone Bargain", meta=(MultiLine=true)) FText CostText;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phone Bargain", meta=(ClampMin="0.01")) float OfferWeight = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phone Bargain") TArray<FChopItStatModifier> Modifiers;
	/** 1 leaves health unchanged. The player's current health percentage is preserved. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phone Bargain", meta=(ClampMin="0.01")) float MaxHealthMultiplier = 1.0f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("PhoneBargain"), GetFName());
	}
};
