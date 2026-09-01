#pragma once

#include "GameFramework/GameStateBase.h"
#include "ChopItGameState.generated.h"

class UChopItQuotaComponent;
class UChopItCycleStateMachineComponent;
class UChopItRunStateComponent;
class UChopItWorldPresentationComponent;
class UChopItEnemyDirectorComponent;
class UChopItEliteEncounterComponent;
class UChopItDayDefinition;

/** Observable owner for run-wide state introduced in later phases. */
UCLASS()
class CHOPIT_API AChopItGameState final : public AGameStateBase
{
	GENERATED_BODY()

public:
	AChopItGameState();
	virtual void BeginPlay() override;
	UChopItQuotaComponent* GetQuotaComponent() const { return QuotaComponent; }
	UChopItCycleStateMachineComponent* GetCycleStateMachine() const { return CycleStateMachine; }
	UChopItRunStateComponent* GetRunStateComponent() const { return RunStateComponent; }
	UChopItEnemyDirectorComponent* GetEnemyDirectorComponent() const { return EnemyDirectorComponent; }
	static int32 ResolveQuotaTarget(int32 DayNumber, const UChopItDayDefinition* DayDefinition);

private:
	UFUNCTION() void HandleRunResult(EChopItRunResult Result, int32 DayNumber);
	int32 ResolveInitialDay(const UChopItDayDefinition* DayDefinition) const;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<UChopItQuotaComponent> QuotaComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Cycle")
	TObjectPtr<UChopItRunStateComponent> RunStateComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Cycle")
	TObjectPtr<UChopItCycleStateMachineComponent> CycleStateMachine;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Cycle")
	TObjectPtr<UChopItWorldPresentationComponent> WorldPresentationComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Enemies")
	TObjectPtr<UChopItEnemyDirectorComponent> EnemyDirectorComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Enemies")
	TObjectPtr<UChopItEliteEncounterComponent> EliteEncounterComponent;
};
