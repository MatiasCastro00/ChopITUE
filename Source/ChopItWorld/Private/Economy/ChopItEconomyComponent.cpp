#include "Economy/ChopItEconomyComponent.h"

UChopItEconomyComponent::UChopItEconomyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UChopItEconomyComponent::ApplyTransaction(
	const FGuid TransactionId,
	const FName Reason,
	const int64 Delta)
{
	if (!TransactionId.IsValid() || Delta == 0 || ProcessedTransactions.Contains(TransactionId))
	{
		return false;
	}
	if ((Delta > 0 && Balance > MAX_int64 - Delta)
		|| (Delta < 0 && (Delta == MIN_int64 || Balance < -Delta)))
	{
		return false;
	}

	Balance += Delta;
	ProcessedTransactions.Add(TransactionId);
	Ledger.Add({ TransactionId, Reason, Delta, Balance });
	OnBalanceChanged.Broadcast(Balance, Delta);
	return true;
}
