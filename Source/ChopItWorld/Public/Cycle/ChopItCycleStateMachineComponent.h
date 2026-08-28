#pragma once

#include "Components/ActorComponent.h"
#include "ChopItCycleStateMachineComponent.generated.h"

class UChopItDayDefinition;
class UChopItQuotaComponent;
class UChopItRunStateComponent;

UENUM(BlueprintType)
enum class EChopItCyclePhase : uint8
{
	Bootstrap,
	Day,
	Dusk,
	Night,
	Elite,
	Resolution,
	Death,
	Victory
};

USTRUCT(BlueprintType)
struct CHOPITWORLD_API FChopItCycleTimings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DayDuration = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DuskMinimumDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DuskHardDeadline = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float NightMinimumDuration = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ElitePlaceholderDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ResolutionDuration = 2.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FChopItCyclePhaseChanged,
	EChopItCyclePhase, NewPhase,
	EChopItCyclePhase, PreviousPhase,
	int32, Generation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChopItCycleClockChanged, EChopItCyclePhase, Phase, float, RemainingSeconds);

/** Sole authority for cycle transitions and phase-owned timers. */
UCLASS(ClassGroup = (ChopIt), BlueprintType, meta = (BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItCycleStateMachineComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItCycleStateMachineComponent();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ConfigureFromDay(const UChopItDayDefinition* DayDefinition);
	bool StartCycle();
	bool RequestLever(AActor* Interactor);
	bool RequestDeath(AActor* Cause);
	bool NotifyEliteDefeated(AActor* DefeatedElite);
	bool RequestVictoryRetirement();
	bool RequestInfiniteMode();

	EChopItCyclePhase GetCurrentPhase() const { return CurrentPhase; }
	uint32 GetPhaseGeneration() const { return PhaseGeneration; }
	float GetPhaseElapsed() const;
	float GetPhaseRemaining() const;
	bool IsNightMinimumElapsed() const { return bNightMinimumElapsed; }
	bool IsInfiniteMode() const;
	const FChopItCycleTimings& GetTimings() const { return Timings; }

	static bool IsTransitionAllowed(EChopItCyclePhase From, EChopItCyclePhase To);
	static bool CanAcceptLever(EChopItCyclePhase Phase, bool bQuotaComplete, bool bMinimumNightElapsed);
	static EChopItCyclePhase ResolveDuskDeadline(bool bQuotaComplete);

#if WITH_DEV_AUTOMATION_TESTS
	bool TransitionForAutomation(EChopItCyclePhase NewPhase) { return TransitionTo(NewPhase); }
	bool IsGenerationCurrentForAutomation(uint32 ExpectedGeneration) const { return IsCurrentGeneration(ExpectedGeneration); }
#endif

	UPROPERTY(BlueprintAssignable, Category = "ChopIt|Cycle")
	FChopItCyclePhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "ChopIt|Cycle")
	FChopItCycleClockChanged OnClockChanged;

private:
	UFUNCTION()
	void HandleQuotaChanged(int32 Progress, int32 Target, bool bComplete);

	bool TransitionTo(EChopItCyclePhase NewPhase);
	void EnterCurrentPhase();
	void ClearPhaseTimers();
	void BroadcastClock();
	bool IsCurrentGeneration(uint32 ExpectedGeneration) const;
	UChopItQuotaComponent* ResolveQuota() const;
	UChopItRunStateComponent* ResolveRunState() const;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Cycle")
	EChopItCyclePhase CurrentPhase = EChopItCyclePhase::Bootstrap;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Cycle")
	FChopItCycleTimings Timings;

	uint32 PhaseGeneration = 0;
	double PhaseStartedAt = 0.0;
	bool bDuskMinimumElapsed = false;
	bool bNightMinimumElapsed = false;
	FTimerHandle PhaseTimerHandle;
	FTimerHandle DeadlineTimerHandle;
	FTimerHandle ClockTimerHandle;
};
