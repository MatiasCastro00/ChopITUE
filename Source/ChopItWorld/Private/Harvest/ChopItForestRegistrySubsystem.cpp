#include "Harvest/ChopItForestRegistrySubsystem.h"

#include "Harvest/ChopItLogPickup.h"
#include "Harvest/ChopItTree.h"

namespace
{
	template <typename ActorType>
	int32 CompactAndCount(TArray<TWeakObjectPtr<ActorType>>& Actors)
	{
		Actors.RemoveAllSwap([](const TWeakObjectPtr<ActorType>& Actor) { return !Actor.IsValid(); });
		return Actors.Num();
	}
}

void UChopItForestRegistrySubsystem::RegisterTree(AChopItTree* Tree)
{
	if (IsValid(Tree))
	{
		Trees.AddUnique(Tree);
	}
}

void UChopItForestRegistrySubsystem::UnregisterTree(AChopItTree* Tree)
{
	Trees.Remove(Tree);
}

void UChopItForestRegistrySubsystem::RegisterLogPickup(AChopItLogPickup* Pickup)
{
	if (IsValid(Pickup))
	{
		LogPickups.AddUnique(Pickup);
	}
}

void UChopItForestRegistrySubsystem::UnregisterLogPickup(AChopItLogPickup* Pickup)
{
	LogPickups.Remove(Pickup);
}

int32 UChopItForestRegistrySubsystem::GetTreeCount()
{
	return CompactAndCount(Trees);
}

int32 UChopItForestRegistrySubsystem::GetLogPickupCount()
{
	return CompactAndCount(LogPickups);
}
