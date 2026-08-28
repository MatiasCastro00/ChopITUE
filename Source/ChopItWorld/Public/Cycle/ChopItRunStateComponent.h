#pragma once

#include "Components/ActorComponent.h"
#include "ChopItRunStateComponent.generated.h"

UENUM(BlueprintType)
enum class EChopItRunResult : uint8
{
	None,
	CycleCompleted,
	QuotaFailed,
	Defeat,
	Victory
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChopItRunResultChanged, EChopItRunResult, Result, int32, DayNumber);

/** Run-scoped day/result state. Phase transitions remain owned by the cycle FSM. */
UCLASS(ClassGroup = (ChopIt), BlueprintType, meta = (BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItRunStateComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItRunStateComponent();
	void InitializeRun(int32 InitialDay = 1);
	void MarkCycleCompleted();
	void MarkQuotaFailed();
	void MarkDefeat();
	void MarkVictory();
	void AdvanceDay();
	void EnterInfiniteMode();

	int32 GetDayNumber() const { return DayNumber; }
	EChopItRunResult GetResult() const { return Result; }
	bool IsInfiniteMode() const { return bInfiniteMode; }

	UPROPERTY(BlueprintAssignable, Category = "ChopIt|Run")
	FChopItRunResultChanged OnResultChanged;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Run")
	int32 DayNumber = 1;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Run")
	EChopItRunResult Result = EChopItRunResult::None;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Run")
	bool bInfiniteMode = false;
};
