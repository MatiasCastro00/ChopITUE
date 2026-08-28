#include "Combat/ChopItHealthComponent.h"

#include "Targeting/ChopItTargetingSubsystem.h"

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
	}
	return Damage;
}

void UChopItHealthComponent::ResetHealth()
{
	CurrentHealth = FMath::Max(1.0f, MaxHealth);
	bDeathBroadcast = false;
}

void UChopItHealthComponent::SetMaxHealth(const float NewMaxHealth)
{
	MaxHealth = FMath::Max(1.0f, NewMaxHealth);
	ResetHealth();
}
