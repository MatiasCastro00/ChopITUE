#include "Combat/ChopItHealthComponent.h"

#include "Targeting/ChopItTargetingSubsystem.h"
#include "Items/ChopItItemEventSubsystem.h"

UChopItHealthComponent::UChopItHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetHealth();
	if (UWorld* World = GetWorld())
	{
		World->GetSubsystem<UChopItTargetingSubsystem>()->RegisterTarget(this);
	}
}

void UChopItHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UChopItTargetingSubsystem* Targeting = World->GetSubsystem<UChopItTargetingSubsystem>())
		{
			Targeting->UnregisterTarget(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

float UChopItHealthComponent::ApplyDamage(const FChopItDamageSpec& DamageSpec, AActor* DamageSource, const FVector& ImpactLocation)
{
	if (!IsAlive())
	{
		return 0.0f;
	}

	const float Damage = FMath::Min(CurrentHealth, DamageSpec.CalculateFinalDamage());
	if (Damage <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHealth -= Damage;
	const FVector ResolvedImpact = ImpactLocation.IsNearlyZero() && GetOwner() ? GetOwner()->GetActorLocation() : ImpactLocation;
	OnDamageReceived.Broadcast(Damage, DamageSpec.bCritical, DamageSource, ResolvedImpact);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, DamageSource);
	if (!IsAlive() && !bDeathBroadcast)
	{
		bDeathBroadcast = true;
		OnDeath.Broadcast(GetOwner(), DamageSource);
		if (DamageSource && TargetKind != EChopItDamageTargetKind::Other)
		{
			FChopItItemEventContext Death;
			Death.Event = TargetKind == EChopItDamageTargetKind::Enemy ? EChopItItemEvent::EnemyKilled : EChopItItemEvent::TreeDestroyed;
			Death.Recipient = DamageSource; Death.Source = DamageSource; Death.Target = GetOwner();
			Death.Amount = Damage; Death.Location = ResolvedImpact; Death.bExecution = DamageSpec.bExecution;
			UChopItItemEventSubsystem::Emit(this, Death);
		}
	}
	FChopItItemEventContext Event;
	Event.Source = DamageSource; Event.Target = GetOwner(); Event.Amount = Damage;
	Event.Location = ResolvedImpact; Event.bCritical = DamageSpec.bCritical; Event.bExecution = DamageSpec.bExecution;
	if (GetOwner())
	{
		Event.Event = EChopItItemEvent::DamageReceived; Event.Recipient = GetOwner();
		UChopItItemEventSubsystem::Emit(this, Event);
	}
	if (DamageSource)
	{
		Event.Recipient = DamageSource; Event.Event = EChopItItemEvent::DamageDealt;
		UChopItItemEventSubsystem::Emit(this, Event);
		if (TargetKind == EChopItDamageTargetKind::Tree)
		{
			Event.Event = EChopItItemEvent::TreeHit;
			UChopItItemEventSubsystem::Emit(this, Event);
		}
	}
	return Damage;
}

float UChopItHealthComponent::Heal(float Amount, AActor* Source)
{
	if (!IsAlive() || !FMath::IsFinite(Amount) || Amount <= 0.f) return 0.f;
	const float Applied = FMath::Min(Amount, MaxHealth - CurrentHealth);
	if (Applied > 0.f) { CurrentHealth += Applied; OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, Source); }
	return Applied;
}

void UChopItHealthComponent::ResetHealth()
{
	CurrentHealth = FMath::Max(1.0f, MaxHealth);
	bDeathBroadcast = false;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, nullptr);
}

void UChopItHealthComponent::SetMaxHealth(const float NewMaxHealth)
{
	MaxHealth = FMath::Max(1.0f, NewMaxHealth);
	ResetHealth();
}
