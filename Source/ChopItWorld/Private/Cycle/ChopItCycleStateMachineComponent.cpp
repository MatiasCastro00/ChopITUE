#include "Cycle/ChopItCycleStateMachineComponent.h"

#include "ChopItLogChannels.h"
#include "Cycle/ChopItRunStateComponent.h"
#include "Economy/ChopItDayDefinition.h"
#include "Economy/ChopItQuotaComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UChopItCycleStateMachineComponent::UChopItCycleStateMachineComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItCycleStateMachineComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPhaseTimers();
	Super::EndPlay(EndPlayReason);
}

void UChopItCycleStateMachineComponent::ConfigureFromDay(const UChopItDayDefinition* DayDefinition)
{
	if (!DayDefinition)
	{
		return;
	}
	Timings.DayDuration = FMath::Max(0.1f, DayDefinition->DayDuration);
	Timings.DuskMinimumDuration = FMath::Max(0.0f, DayDefinition->DuskMinimumDuration);
	Timings.DuskHardDeadline = FMath::Max(Timings.DuskMinimumDuration, DayDefinition->DuskHardDeadline);
	Timings.NightMinimumDuration = FMath::Max(0.0f, DayDefinition->NightMinimumDuration);
	Timings.ElitePlaceholderDuration = FMath::Max(0.1f, DayDefinition->ElitePlaceholderDuration);
	Timings.ResolutionDuration = FMath::Max(0.1f, DayDefinition->ResolutionDuration);
}

bool UChopItCycleStateMachineComponent::StartCycle()
{
	if (CurrentPhase != EChopItCyclePhase::Bootstrap && CurrentPhase != EChopItCyclePhase::Resolution)
	{
		return false;
	}
	if (CurrentPhase == EChopItCyclePhase::Resolution)
	{
		if (UChopItRunStateComponent* RunState = ResolveRunState())
		{
			RunState->AdvanceDay();
		}
		if (UChopItQuotaComponent* Quota = ResolveQuota())
		{
			Quota->InitializeQuota(Quota->GetTarget() + 1);
		}
	}
	if (UChopItQuotaComponent* Quota = ResolveQuota())
	{
		Quota->OnQuotaChanged.AddUniqueDynamic(this, &UChopItCycleStateMachineComponent::HandleQuotaChanged);
	}
	return TransitionTo(EChopItCyclePhase::Day);
}

void UChopItCycleStateMachineComponent::HandleQuotaChanged(const int32, const int32, const bool bComplete)
{
	if (CurrentPhase == EChopItCyclePhase::Dusk && bDuskMinimumElapsed && bComplete)
	{
		TransitionTo(EChopItCyclePhase::Night);
	}
}

bool UChopItCycleStateMachineComponent::RequestLever(AActor* Interactor)
{
	const UChopItQuotaComponent* Quota = ResolveQuota();
	if (!CanAcceptLever(CurrentPhase, Quota && Quota->IsComplete(), bNightMinimumElapsed))
	{
		UE_LOG(LogChopIt, Display, TEXT("Lever rejected for %s: phase=%d quota=%s night-minimum=%s."),
			*GetNameSafe(Interactor), static_cast<int32>(CurrentPhase),
			Quota && Quota->IsComplete() ? TEXT("complete") : TEXT("incomplete"),
			bNightMinimumElapsed ? TEXT("yes") : TEXT("no"));
		return false;
	}
	UE_LOG(LogChopIt, Display, TEXT("Lever accepted for %s."), *GetNameSafe(Interactor));
	return TransitionTo(EChopItCyclePhase::Elite);
}

bool UChopItCycleStateMachineComponent::RequestDeath(AActor* Cause)
{
	if (!IsTransitionAllowed(CurrentPhase, EChopItCyclePhase::Death))
	{
		return false;
	}
	UE_LOG(LogChopIt, Display, TEXT("Run death requested by %s."), *GetNameSafe(Cause));
	if (UChopItRunStateComponent* RunState = ResolveRunState())
	{
		RunState->MarkDefeat();
	}
	return TransitionTo(EChopItCyclePhase::Death);
}

bool UChopItCycleStateMachineComponent::NotifyEliteDefeated(AActor* DefeatedElite)
{
	if (CurrentPhase != EChopItCyclePhase::Elite)
	{
		return false;
	}
	const UChopItRunStateComponent* RunState = ResolveRunState();
	const EChopItCyclePhase NextPhase = RunState && RunState->GetDayNumber() >= 7
		? EChopItCyclePhase::Victory : EChopItCyclePhase::Resolution;
	UE_LOG(LogChopIt, Display, TEXT("Elite %s defeated; moving to phase %d."), *GetNameSafe(DefeatedElite), static_cast<int32>(NextPhase));
	return TransitionTo(NextPhase);
}

bool UChopItCycleStateMachineComponent::RequestVictoryRetirement()
{
	if (CurrentPhase != EChopItCyclePhase::Victory)
	{
		return false;
	}
	if (UChopItRunStateComponent* RunState = ResolveRunState())
	{
		RunState->MarkVictory();
		return true;
	}
	return false;
}

bool UChopItCycleStateMachineComponent::RequestInfiniteMode()
{
	if (CurrentPhase != EChopItCyclePhase::Victory)
	{
		return false;
	}
	UChopItRunStateComponent* RunState = ResolveRunState();
	if (!RunState)
	{
		return false;
	}
	RunState->EnterInfiniteMode();
	UE_LOG(LogChopIt, Display, TEXT("Infinite night accepted after day %d."), RunState->GetDayNumber());
	return TransitionTo(EChopItCyclePhase::Night);
}

bool UChopItCycleStateMachineComponent::IsInfiniteMode() const
{
	const UChopItRunStateComponent* RunState = ResolveRunState();
	return RunState && RunState->IsInfiniteMode();
}

float UChopItCycleStateMachineComponent::GetPhaseElapsed() const
{
	const UWorld* World = GetWorld();
	return World ? FMath::Max(0.0, World->GetTimeSeconds() - PhaseStartedAt) : 0.0f;
}

float UChopItCycleStateMachineComponent::GetPhaseRemaining() const
{
	const float Elapsed = GetPhaseElapsed();
	switch (CurrentPhase)
	{
	case EChopItCyclePhase::Day: return FMath::Max(0.0f, Timings.DayDuration - Elapsed);
	case EChopItCyclePhase::Dusk: return FMath::Max(0.0f, Timings.DuskHardDeadline - Elapsed);
	case EChopItCyclePhase::Night: return IsInfiniteMode() ? -1.0f : FMath::Max(0.0f, Timings.NightMinimumDuration - Elapsed);
	case EChopItCyclePhase::Elite: return FMath::Max(0.0f, Timings.ElitePlaceholderDuration - Elapsed);
	case EChopItCyclePhase::Resolution: return FMath::Max(0.0f, Timings.ResolutionDuration - Elapsed);
	default: return -1.0f;
	}
}

bool UChopItCycleStateMachineComponent::IsTransitionAllowed(
	const EChopItCyclePhase From,
	const EChopItCyclePhase To)
{
	if (To == EChopItCyclePhase::Death)
	{
		return From == EChopItCyclePhase::Day || From == EChopItCyclePhase::Dusk
			|| From == EChopItCyclePhase::Night || From == EChopItCyclePhase::Elite;
	}
	switch (From)
	{
	case EChopItCyclePhase::Bootstrap: return To == EChopItCyclePhase::Day;
	case EChopItCyclePhase::Day: return To == EChopItCyclePhase::Dusk;
	case EChopItCyclePhase::Dusk: return To == EChopItCyclePhase::Night;
	case EChopItCyclePhase::Night: return To == EChopItCyclePhase::Elite;
	case EChopItCyclePhase::Elite: return To == EChopItCyclePhase::Resolution || To == EChopItCyclePhase::Victory;
	case EChopItCyclePhase::Resolution: return To == EChopItCyclePhase::Day;
	case EChopItCyclePhase::Victory: return To == EChopItCyclePhase::Day;
	default: return false;
	}
}

bool UChopItCycleStateMachineComponent::CanAcceptLever(
	const EChopItCyclePhase Phase,
	const bool bQuotaComplete,
	const bool bMinimumNightElapsed)
{
	// The night already has an automatic elite countdown. Once the quota is paid,
	// the player may use the lever to trigger that encounter early instead.
	return Phase == EChopItCyclePhase::Night && bQuotaComplete;
}

EChopItCyclePhase UChopItCycleStateMachineComponent::ResolveDuskDeadline(const bool bQuotaComplete)
{
	// Unpaid quota is pressure, not an instant loss. The player may keep
	// gathering and delivering during the increasingly dangerous night.
	return EChopItCyclePhase::Night;
}

bool UChopItCycleStateMachineComponent::TransitionTo(const EChopItCyclePhase NewPhase)
{
	if (!IsTransitionAllowed(CurrentPhase, NewPhase))
	{
		return false;
	}
	const EChopItCyclePhase PreviousPhase = CurrentPhase;
	ClearPhaseTimers();
	CurrentPhase = NewPhase;
	++PhaseGeneration;
	PhaseStartedAt = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	bDuskMinimumElapsed = false;
	bNightMinimumElapsed = false;
	UE_LOG(LogChopIt, Display, TEXT("Cycle phase: %d -> %d generation=%u."),
		static_cast<int32>(PreviousPhase), static_cast<int32>(CurrentPhase), PhaseGeneration);
	OnPhaseChanged.Broadcast(CurrentPhase, PreviousPhase, static_cast<int32>(PhaseGeneration));
	EnterCurrentPhase();
	return true;
}

void UChopItCycleStateMachineComponent::EnterCurrentPhase()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const uint32 ExpectedGeneration = PhaseGeneration;
	auto IsCurrent = [this, ExpectedGeneration]() { return IsCurrentGeneration(ExpectedGeneration); };

	switch (CurrentPhase)
	{
	case EChopItCyclePhase::Day:
	{
		FTimerDelegate Callback = FTimerDelegate::CreateWeakLambda(this, [this, IsCurrent]()
		{
			if (IsCurrent()) { TransitionTo(EChopItCyclePhase::Dusk); }
		});
		World->GetTimerManager().SetTimer(PhaseTimerHandle, Callback, Timings.DayDuration, false);
		break;
	}
	case EChopItCyclePhase::Dusk:
	{
		FTimerDelegate MinimumCallback = FTimerDelegate::CreateWeakLambda(this, [this, IsCurrent]()
		{
			if (!IsCurrent()) { return; }
			bDuskMinimumElapsed = true;
			if (const UChopItQuotaComponent* Quota = ResolveQuota(); Quota && Quota->IsComplete())
			{
				TransitionTo(EChopItCyclePhase::Night);
			}
		});
		World->GetTimerManager().SetTimer(PhaseTimerHandle, MinimumCallback, Timings.DuskMinimumDuration, false);

		FTimerDelegate DeadlineCallback = FTimerDelegate::CreateWeakLambda(this, [this, IsCurrent]()
		{
			if (!IsCurrent()) { return; }
			const UChopItQuotaComponent* Quota = ResolveQuota();
			const EChopItCyclePhase DeadlineResult = ResolveDuskDeadline(Quota && Quota->IsComplete());
			TransitionTo(DeadlineResult);
		});
		World->GetTimerManager().SetTimer(DeadlineTimerHandle, DeadlineCallback, Timings.DuskHardDeadline, false);
		break;
	}
	case EChopItCyclePhase::Night:
	{
		if (IsInfiniteMode())
		{
			break;
		}
		FTimerDelegate Callback = FTimerDelegate::CreateWeakLambda(this, [this, IsCurrent]()
		{
			if (IsCurrent()) { TransitionTo(EChopItCyclePhase::Elite); }
		});
		World->GetTimerManager().SetTimer(PhaseTimerHandle, Callback, Timings.NightMinimumDuration, false);
		break;
	}
	case EChopItCyclePhase::Elite:
		break;
	case EChopItCyclePhase::Resolution:
	{
		if (UChopItRunStateComponent* RunState = ResolveRunState()) { RunState->MarkCycleCompleted(); }
		FTimerDelegate Callback = FTimerDelegate::CreateWeakLambda(this, [this, IsCurrent]()
		{
			if (IsCurrent()) { StartCycle(); }
		});
		World->GetTimerManager().SetTimer(PhaseTimerHandle, Callback, Timings.ResolutionDuration, false);
		break;
	}
	case EChopItCyclePhase::Victory:
		break;
	default:
		break;
	}

	BroadcastClock();
	FTimerDelegate ClockCallback = FTimerDelegate::CreateWeakLambda(this, [this, IsCurrent]()
	{
		if (IsCurrent()) { BroadcastClock(); }
	});
	World->GetTimerManager().SetTimer(ClockTimerHandle, ClockCallback, 0.25f, true);
}

void UChopItCycleStateMachineComponent::ClearPhaseTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
		World->GetTimerManager().ClearTimer(DeadlineTimerHandle);
		World->GetTimerManager().ClearTimer(ClockTimerHandle);
	}
}

void UChopItCycleStateMachineComponent::BroadcastClock()
{
	OnClockChanged.Broadcast(CurrentPhase, GetPhaseRemaining());
}

bool UChopItCycleStateMachineComponent::IsCurrentGeneration(const uint32 ExpectedGeneration) const
{
	return PhaseGeneration == ExpectedGeneration;
}

UChopItQuotaComponent* UChopItCycleStateMachineComponent::ResolveQuota() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UChopItQuotaComponent>() : nullptr;
}

UChopItRunStateComponent* UChopItCycleStateMachineComponent::ResolveRunState() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UChopItRunStateComponent>() : nullptr;
}
