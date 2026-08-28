#pragma once

#include "Components/ActorComponent.h"
#include "ChopItQuotaComponent.generated.h"

USTRUCT(BlueprintType)
struct CHOPITWORLD_API FChopItQuotaTransferResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 Requested = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Accepted = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Remainder = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bCompletedNow = false;

	UPROPERTY(BlueprintReadOnly)
	bool bDuplicate = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FChopItQuotaChanged, int32, Progress, int32, Target, bool, bComplete);

/** Authoritative, integer and idempotent daily quota state. */
UCLASS(ClassGroup = (ChopIt), BlueprintType, meta = (BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItQuotaComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItQuotaComponent();

	UFUNCTION(BlueprintCallable, Category = "ChopIt|Quota")
	void InitializeQuota(int32 NewTarget);

	UFUNCTION(BlueprintCallable, Category = "ChopIt|Quota")
	FChopItQuotaTransferResult TryContributeWood(FGuid TransactionId, int32 RequestedUnits);

	int32 GetTarget() const { return Target; }
	int32 GetProgress() const { return Progress; }
	int32 GetRemaining() const { return FMath::Max(0, Target - Progress); }
	bool IsComplete() const { return Target > 0 && Progress >= Target; }
	int32 GetProcessedTransactionCount() const { return ProcessedTransactions.Num(); }

	UPROPERTY(BlueprintAssignable, Category = "ChopIt|Quota")
	FChopItQuotaChanged OnQuotaChanged;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Quota")
	int32 Target = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Quota")
	int32 Progress = 0;

	TSet<FGuid> ProcessedTransactions;
};
