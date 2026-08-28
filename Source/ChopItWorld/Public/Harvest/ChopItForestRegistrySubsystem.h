#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "ChopItForestRegistrySubsystem.generated.h"

class AChopItLogPickup;
class AChopItTree;

UCLASS()
class CHOPITWORLD_API UChopItForestRegistrySubsystem final : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterTree(AChopItTree* Tree);
	void UnregisterTree(AChopItTree* Tree);
	void RegisterLogPickup(AChopItLogPickup* Pickup);
	void UnregisterLogPickup(AChopItLogPickup* Pickup);
	int32 GetTreeCount();
	int32 GetLogPickupCount();

private:
	TArray<TWeakObjectPtr<AChopItTree>> Trees;
	TArray<TWeakObjectPtr<AChopItLogPickup>> LogPickups;
};
