#pragma once

#include "Components/ActorComponent.h"
#include "ChopItTetherReceiverComponent.generated.h"

class AChopItQuotaMachine;

/**
 * Player-side tether response. Runs the machine solve after character movement.
 * Input filtering consumes the same physical endpoint and tension state.
 */
UCLASS(ClassGroup = (ChopIt), meta = (BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItTetherReceiverComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItTetherReceiverComponent();
	void BindMachine(AChopItQuotaMachine* InMachine);

	void SetTetherState(
		const FVector& InGuidePoint,
		float InTensionAlpha,
		bool bInHardLimit,
		float InPullAcceleration,
		float InPullDamping);
	void ClearTetherState();

	/** Removes only input that lengthens the final route span. */
	FVector ConstrainMovementDirection(const FVector& WorldDirection) const;
	bool IsHardLimited() const { return bHardLimit; }
	float GetTensionAlpha() const { return TensionAlpha; }

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AChopItQuotaMachine> Machine;
	FVector GetOutwardDirection() const;

	FVector GuidePoint = FVector::ZeroVector;
	float TensionAlpha = 0.0f;
	float PullAcceleration = 0.0f;
	float PullDamping = 0.0f;
	bool bHardLimit = false;
	bool bHasTetherState = false;
};
