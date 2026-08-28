#pragma once

#include "Components/ActorComponent.h"
#include "ChopItEconomyComponent.generated.h"

USTRUCT(BlueprintType)
struct CHOPITWORLD_API FChopItEconomyTransaction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGuid TransactionId;

	UPROPERTY(BlueprintReadOnly)
	FName Reason;

	UPROPERTY(BlueprintReadOnly)
	int64 Delta = 0;

	UPROPERTY(BlueprintReadOnly)
	int64 BalanceAfter = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChopItBalanceChanged, int64, Balance, int64, Delta);

/** Owns the non-negative run balance and an idempotent transaction ledger. */
UCLASS(ClassGroup = (ChopIt), BlueprintType, meta = (BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItEconomyComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItEconomyComponent();

	UFUNCTION(BlueprintCallable, Category = "ChopIt|Economy")
	bool ApplyTransaction(FGuid TransactionId, FName Reason, int64 Delta);

	int64 GetBalance() const { return Balance; }
	const TArray<FChopItEconomyTransaction>& GetLedger() const { return Ledger; }
	bool HasProcessedTransaction(const FGuid& TransactionId) const { return ProcessedTransactions.Contains(TransactionId); }

	UPROPERTY(BlueprintAssignable, Category = "ChopIt|Economy")
	FChopItBalanceChanged OnBalanceChanged;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Economy")
	int64 Balance = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Economy")
	TArray<FChopItEconomyTransaction> Ledger;

	TSet<FGuid> ProcessedTransactions;
};
