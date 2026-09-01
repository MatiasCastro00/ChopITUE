#pragma once

#include "Components/ActorComponent.h"
#include "ChopItWoodCargoComponent.generated.h"

USTRUCT(BlueprintType)
struct CHOPITWORLD_API FChopItWoodTransferResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 Requested = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Transferred = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Remainder = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChopItWoodCargoChanged, int32, CurrentWood, int32, Capacity);

UCLASS(ClassGroup = (ChopIt), BlueprintType, meta = (BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItWoodCargoComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItWoodCargoComponent();

	UFUNCTION(BlueprintCallable, Category = "ChopIt|Wood")
	FChopItWoodTransferResult TryAddWood(int32 RequestedUnits);

	UFUNCTION(BlueprintCallable, Category = "ChopIt|Wood")
	FChopItWoodTransferResult TryRemoveWood(int32 RequestedUnits);

	UFUNCTION(BlueprintCallable, Category = "ChopIt|Wood")
	bool SetCapacity(int32 NewCapacity);

	/** Development-only supply zones may deliberately overfill cargo for stress testing. */
	UFUNCTION(BlueprintCallable, Category = "ChopIt|Wood|Testing")
	FChopItWoodTransferResult GrantWoodForTesting(int32 RequestedUnits);

	int32 GetCurrentWood() const { return CurrentWood; }
	int32 GetCapacity() const { return Capacity; }
	int32 GetAvailableCapacity() const { return FMath::Max(0, Capacity - CurrentWood); }

	UPROPERTY(BlueprintAssignable, Category = "ChopIt|Wood")
	FChopItWoodCargoChanged OnCargoChanged;

private:
	UPROPERTY(EditAnywhere, Category = "ChopIt|Wood", meta = (ClampMin = "0"))
	int32 Capacity = 24;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Wood")
	int32 CurrentWood = 0;
};
