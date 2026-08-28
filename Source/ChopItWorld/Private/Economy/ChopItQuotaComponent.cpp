#include "Economy/ChopItQuotaComponent.h"

UChopItQuotaComponent::UChopItQuotaComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItQuotaComponent::InitializeQuota(const int32 NewTarget)
{
	Target = FMath::Max(0, NewTarget);
	Progress = 0;
	ProcessedTransactions.Reset();
	OnQuotaChanged.Broadcast(Progress, Target, IsComplete());
}

FChopItQuotaTransferResult UChopItQuotaComponent::TryContributeWood(
	const FGuid TransactionId,
	const int32 RequestedUnits)
{
	FChopItQuotaTransferResult Result;
	Result.Requested = FMath::Max(0, RequestedUnits);
	Result.Remainder = Result.Requested;
	if (!TransactionId.IsValid() || Result.Requested == 0)
	{
		return Result;
	}
	if (ProcessedTransactions.Contains(TransactionId))
	{
		Result.bDuplicate = true;
		return Result;
	}

	ProcessedTransactions.Add(TransactionId);
	const bool bWasComplete = IsComplete();
	Result.Accepted = FMath::Min(Result.Requested, GetRemaining());
	Result.Remainder = Result.Requested - Result.Accepted;
	Progress += Result.Accepted;
	Result.bCompletedNow = !bWasComplete && IsComplete();
	if (Result.Accepted > 0)
	{
		OnQuotaChanged.Broadcast(Progress, Target, IsComplete());
	}
	return Result;
}
