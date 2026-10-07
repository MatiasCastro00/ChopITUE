#include "Items/ChopItPassiveEffects.h"
#include "Items/ChopItItemComponent.h"
#include "Items/ChopItItemEventSubsystem.h"
#include "Combat/ChopItHealthComponent.h"
#include "Combat/ChopItCombatStatsComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Items/ChopItMandrakeScream.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

UChopItMandrakeEffect::UChopItMandrakeEffect()
{
	Events = {EChopItItemEvent::EnemyKilled};
	BaseValue = 0.1f;
}
void UChopItMandrakeEffect::HandleEvent_Implementation(const FChopItItemEventContext& Context)
{
	if (!Inventory || !GetWorld() || !IsValid(Context.Target) || Context.Source != Inventory->GetOwner()) return;
	if (FMath::FRand() >= FMath::Clamp(GetStackedValue(), 0.f, 1.f)) return;
	FVector Location = Context.Target->GetActorLocation();
	if (const ACharacter* Character = Cast<ACharacter>(Context.Target))
		Location.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FRotator Facing = (Inventory->GetOwner()->GetActorLocation()-Location).Rotation();
	if (AChopItMandrakeScream* Mandrake = GetWorld()->SpawnActor<AChopItMandrakeScream>(AChopItMandrakeScream::StaticClass(), Location, Facing, Params))
		Mandrake->Initialize(Inventory->GetOwner(), Radius, DamagePerSecond, Duration);
}

UChopItRegenerationEffect::UChopItRegenerationEffect() { BaseValue = 1.f; }
void UChopItRegenerationEffect::OnActivated_Implementation()
{
	Health = Inventory->GetOwner()->FindComponentByClass<UChopItHealthComponent>();
	if (GetWorld()) GetWorld()->GetTimerManager().SetTimer(Timer, this, &ThisClass::Regenerate, FMath::Max(0.05f, Interval), true);
}
void UChopItRegenerationEffect::Regenerate()
{
	if (Health.IsValid()) Health->Heal(GetStackedValue() * FMath::Max(0.05f, Interval), Inventory->GetOwner());
}
void UChopItRegenerationEffect::OnDeactivated_Implementation()
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(Timer);
	Health.Reset();
}

UChopItLifeStealEffect::UChopItLifeStealEffect() { Events = {EChopItItemEvent::DamageDealt}; }
void UChopItLifeStealEffect::OnActivated_Implementation() { Health = Inventory->GetOwner()->FindComponentByClass<UChopItHealthComponent>(); }
void UChopItLifeStealEffect::HandleEvent_Implementation(const FChopItItemEventContext& Context)
{
	if (!Context.bExecution && Context.Target != Inventory->GetOwner() && Health.IsValid())
		Health->Heal(Context.Amount * GetStackedValue(), Inventory->GetOwner());
}

UChopItWoodYieldEffect::UChopItWoodYieldEffect() { BaseValue = 0.1f; }
void UChopItWoodYieldEffect::OnActivated_Implementation()
{
	Stats = Inventory->GetOwner()->FindComponentByClass<UChopItCombatStatsComponent>();
	OnStacksChanged_Implementation();
}
void UChopItWoodYieldEffect::OnStacksChanged_Implementation()
{
	if (!Stats.IsValid()) return;
	Stats->RemoveModifier(ModifierHandle);
	FChopItStatModifier Modifier;
	Modifier.Stat = EChopItCombatStat::WoodYield;
	Modifier.Magnitude = GetStackedValue();
	ModifierHandle = Stats->AddModifier(Modifier);
}
void UChopItWoodYieldEffect::OnDeactivated_Implementation()
{
	if (Stats.IsValid()) Stats->RemoveModifier(ModifierHandle);
	ModifierHandle.Invalidate();
}

UChopItInfestationEffect::UChopItInfestationEffect() { Events = {EChopItItemEvent::DamageDealt}; }
void UChopItInfestationEffect::HandleEvent_Implementation(const FChopItItemEventContext& Context)
{
	if (Context.bExecution || !IsValid(Context.Target) || Context.Target == Inventory->GetOwner()) return;
	ApplyInfestation(Context.Target->FindComponentByClass<UChopItHealthComponent>(), Context.Amount * GetStackedValue());
}
float UChopItInfestationEffect::GetInfestation(UChopItHealthComponent* Target) const
{
	const auto* State = Targets.Find(Target); return State ? State->Amount : 0.f;
}
void UChopItInfestationEffect::ApplyInfestation(UChopItHealthComponent* Target, float Amount)
{
	if (Stacks <= 0 || !IsValid(Target) || !Target->IsAlive() || !FMath::IsFinite(Amount) || Amount <= 0.f) return;
	for (auto It = Targets.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
	FInfestationState& State = Targets.FindOrAdd(Target);
	if (!State.HealthChanged.IsValid())
	{
		const TWeakObjectPtr<UChopItHealthComponent> WeakTarget(Target);
		State.HealthChanged = Target->OnHealthChanged.AddWeakLambda(this, [this, WeakTarget](float, float, AActor*)
		{
			if (WeakTarget.IsValid()) CheckExecution(WeakTarget.Get());
		});
	}
	State.Amount += Amount;
	Notify(Target, State.Amount, EChopItItemEvent::InfestationChanged);
	CheckExecution(Target);
}
void UChopItInfestationEffect::CheckExecution(UChopItHealthComponent* Target)
{
	const auto* State = Targets.Find(Target);
	if (!State || (Target->IsAlive() && Target->GetCurrentHealth() > State->Amount)) return;
	const float Amount = State->Amount;
	Target->OnHealthChanged.Remove(State->HealthChanged);
	Targets.Remove(Target); // Remove before callbacks and damage to prevent recursive executions.
	if (Target->IsAlive())
	{
		FChopItDamageSpec Execution;
		Execution.BaseDamage = Target->GetCurrentHealth();
		Execution.bExecution = true;
		Target->ApplyDamage(Execution, Inventory->GetOwner());
		Notify(Target, Amount, EChopItItemEvent::InfestationExecuted);
	}
	Notify(Target, 0.f, EChopItItemEvent::InfestationChanged);
}
void UChopItInfestationEffect::Notify(UChopItHealthComponent* Target, float Amount, EChopItItemEvent Event)
{
	FChopItItemEventContext Context;
	Context.Event = Event; Context.Recipient = Inventory->GetOwner(); Context.Source = Inventory->GetOwner();
	Context.Target = Target->GetOwner(); Context.Amount = Amount; Context.Item = Definition;
	Context.Location = Context.Target ? Context.Target->GetActorLocation() : FVector::ZeroVector;
	Context.bExecution = Event == EChopItItemEvent::InfestationExecuted;
	UChopItItemEventSubsystem::Emit(this, Context);
}
void UChopItInfestationEffect::OnDeactivated_Implementation()
{
	const auto Snapshot = Targets;
	Targets.Reset();
	for (const auto& Pair : Snapshot)
		if (Pair.Key.IsValid())
		{
			Pair.Key->OnHealthChanged.Remove(Pair.Value.HealthChanged);
			Notify(Pair.Key.Get(), 0.f, EChopItItemEvent::InfestationChanged);
		}
}
