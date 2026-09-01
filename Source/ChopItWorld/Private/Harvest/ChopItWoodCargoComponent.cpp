#include "Harvest/ChopItWoodCargoComponent.h"

UChopItWoodCargoComponent::UChopItWoodCargoComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FChopItWoodTransferResult UChopItWoodCargoComponent::TryAddWood(const int32 RequestedUnits)
{
	FChopItWoodTransferResult Result;
	Result.Requested = FMath::Max(0, RequestedUnits);
	Result.Transferred = FMath::Min(Result.Requested, GetAvailableCapacity());
	Result.Remainder = Result.Requested - Result.Transferred;
	if (Result.Transferred > 0)
	{
		CurrentWood += Result.Transferred;
		OnCargoChanged.Broadcast(CurrentWood, Capacity);
	}
	return Result;
}

FChopItWoodTransferResult UChopItWoodCargoComponent::TryRemoveWood(const int32 RequestedUnits)
{
	FChopItWoodTransferResult Result;
	Result.Requested = FMath::Max(0, RequestedUnits);
	Result.Transferred = FMath::Min(Result.Requested, CurrentWood);
	Result.Remainder = Result.Requested - Result.Transferred;
	if (Result.Transferred > 0)
	{
		CurrentWood -= Result.Transferred;
		OnCargoChanged.Broadcast(CurrentWood, Capacity);
	}
	return Result;
}

bool UChopItWoodCargoComponent::SetCapacity(const int32 NewCapacity)
{
	if (NewCapacity < CurrentWood || NewCapacity < 0)
	{
		return false;
	}
	if (Capacity != NewCapacity)
	{
		Capacity = NewCapacity;
		OnCargoChanged.Broadcast(CurrentWood, Capacity);
	}
	return true;
}

FChopItWoodTransferResult UChopItWoodCargoComponent::GrantWoodForTesting(const int32 RequestedUnits)
{
	FChopItWoodTransferResult Result;
	Result.Requested = FMath::Max(0, RequestedUnits);
	Result.Transferred = Result.Requested;
	if (Result.Transferred > 0 && CurrentWood <= MAX_int32 - Result.Transferred)
	{
		CurrentWood += Result.Transferred;
		OnCargoChanged.Broadcast(CurrentWood, Capacity);
	}
	else if (Result.Transferred > 0)
	{
		Result.Transferred = 0;
		Result.Remainder = Result.Requested;
	}
	return Result;
}
