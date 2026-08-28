#pragma once

#include "Engine/DataAsset.h"
#include "ChopItDayDefinition.generated.h"

/** Immutable economy configuration for one day. */
UCLASS(BlueprintType)
class CHOPITWORLD_API UChopItDayDefinition final : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChopIt|Day", meta = (ClampMin = "1"))
	int32 DayNumber = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChopIt|Day", meta = (ClampMin = "1"))
	int32 WoodQuota = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChopIt|Day", meta = (ClampMin = "1"))
	int64 MoneyPerWood = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChopIt|Day", meta = (ClampMin = "1"))
	int32 TransferBatchSize = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChopIt|Day", meta = (ClampMin = "0.05"))
	float TransferInterval = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChopIt|Cycle", meta = (ClampMin = "1.0"))
	float DayDuration = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChopIt|Cycle", meta = (ClampMin = "0.0"))
	float DuskMinimumDuration = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChopIt|Cycle", meta = (ClampMin = "0.1"))
	float DuskHardDeadline = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChopIt|Cycle", meta = (ClampMin = "0.0"))
	float NightMinimumDuration = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChopIt|Cycle", meta = (ClampMin = "0.1"))
	float ElitePlaceholderDuration = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChopIt|Cycle", meta = (ClampMin = "0.1"))
	float ResolutionDuration = 2.0f;
};
