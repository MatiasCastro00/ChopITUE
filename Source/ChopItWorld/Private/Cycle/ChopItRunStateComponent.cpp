#include "Cycle/ChopItRunStateComponent.h"

UChopItRunStateComponent::UChopItRunStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItRunStateComponent::InitializeRun(const int32 InitialDay)
{
	DayNumber = FMath::Max(1, InitialDay);
	Result = EChopItRunResult::None;
	bInfiniteMode = false;
}

void UChopItRunStateComponent::MarkCycleCompleted()
{
	if (Result == EChopItRunResult::None)
	{
		Result = EChopItRunResult::CycleCompleted;
		OnResultChanged.Broadcast(Result, DayNumber);
	}
}

void UChopItRunStateComponent::MarkQuotaFailed()
{
	if (Result == EChopItRunResult::None)
	{
		Result = EChopItRunResult::QuotaFailed;
		OnResultChanged.Broadcast(Result, DayNumber);
	}
}

void UChopItRunStateComponent::MarkDefeat()
{
	if (Result == EChopItRunResult::None)
	{
		Result = EChopItRunResult::Defeat;
		OnResultChanged.Broadcast(Result, DayNumber);
	}
}

void UChopItRunStateComponent::MarkVictory()
{
	Result = EChopItRunResult::Victory;
	OnResultChanged.Broadcast(Result, DayNumber);
}

void UChopItRunStateComponent::AdvanceDay()
{
	++DayNumber;
	Result = EChopItRunResult::None;
}

void UChopItRunStateComponent::EnterInfiniteMode()
{
	bInfiniteMode = true;
}
