#include "Harvest/ChopItWoodCargoComponent.h"
#include "Combat/ChopItCombatStatsComponent.h"
#include "Items/ChopItItemEventSubsystem.h"
#include "GameFramework/Actor.h"

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
	if (Result.Requested > 0 && Result.Transferred == 0) OnPickupRejected.Broadcast(Result.Requested);
	if (Result.Transferred > 0)
	{
		float Yield = 1.f;
		if (const auto* Stats = GetOwner() ? GetOwner()->FindComponentByClass<UChopItCombatStatsComponent>() : nullptr)
			Yield = Stats->EvaluateStat(EChopItCombatStat::WoodYield, 1.f);
		const double Bonus = BonusFraction + Result.Transferred * static_cast<double>(FMath::Max(0.f, Yield - 1.f));
		const double WholeBonus = FMath::FloorToDouble(Bonus);
		BonusFraction = Bonus - WholeBonus;
		Result.BonusUnits = static_cast<int32>(FMath::Min(WholeBonus, static_cast<double>(GetAvailableCapacity() - Result.Transferred)));
		CurrentWood += Result.Transferred + Result.BonusUnits;
		OnCargoChanged.Broadcast(CurrentWood, Capacity);
		FChopItItemEventContext Event;
		Event.Event = EChopItItemEvent::WoodCollected; Event.Recipient = GetOwner(); Event.Target = GetOwner();
		Event.Amount = Result.Transferred + Result.BonusUnits;
		UChopItItemEventSubsystem::Emit(this, Event);
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
