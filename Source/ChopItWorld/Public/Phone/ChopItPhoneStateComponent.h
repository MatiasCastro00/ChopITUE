#pragma once

#include "Components/ActorComponent.h"
#include "ChopItPhoneStateComponent.generated.h"

UENUM(BlueprintType)
enum class EChopItPhoneState : uint8
{
	Available,
	Ringing,
	Answered,
	Transforming,
	Dialogue,
	Completed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChopItPhoneStateChanged, EChopItPhoneState, Previous,
	EChopItPhoneState, Current);

/** Authoritative phone lifecycle. Presentation listens to changes; it cannot bypass transitions. */
UCLASS(ClassGroup=(ChopIt), BlueprintType, meta=(BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItPhoneStateComponent final : public UActorComponent
{
	GENERATED_BODY()
public:
	UChopItPhoneStateComponent();

	UFUNCTION(BlueprintPure, Category="ChopIt|Phone")
	EChopItPhoneState GetState() const { return State; }
	UFUNCTION(BlueprintCallable, Category="ChopIt|Phone")
	bool TryTransition(EChopItPhoneState Next);

	UPROPERTY(BlueprintAssignable, Category="Phone State")
	FChopItPhoneStateChanged OnStateChanged;

private:
	UPROPERTY(VisibleInstanceOnly, Category="Phone State")
	EChopItPhoneState State = EChopItPhoneState::Available;
};
