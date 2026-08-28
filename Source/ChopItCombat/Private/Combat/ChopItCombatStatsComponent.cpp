#include "Combat/ChopItCombatStatsComponent.h"

UChopItCombatStatsComponent::UChopItCombatStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FGuid UChopItCombatStatsComponent::AddModifier(const FChopItStatModifier& Modifier)
{
	const FGuid Handle = FGuid::NewGuid();
	Modifiers.Add(Handle, Modifier);
	OnStatsChanged.Broadcast();
	return Handle;
}

bool UChopItCombatStatsComponent::RemoveModifier(const FGuid& Handle)
{
	const bool bRemoved = Modifiers.Remove(Handle) > 0;
	if (bRemoved)
	{
		OnStatsChanged.Broadcast();
	}
	return bRemoved;
}

float UChopItCombatStatsComponent::EvaluateStat(const EChopItCombatStat Stat, const float BaseValue) const
{
	float Additive = 0.0f;
	float Multiplicative = 1.0f;
	for (const TPair<FGuid, FChopItStatModifier>& Pair : Modifiers)
	{
		const FChopItStatModifier& Modifier = Pair.Value;
		if (Modifier.Stat != Stat)
		{
			continue;
		}

		if (Modifier.Operation == EChopItModifierOperation::Add)
		{
			Additive += Modifier.Magnitude;
		}
		else
		{
			Multiplicative *= Modifier.Magnitude;
		}
	}

	return FMath::Max(0.0f, (BaseValue + Additive) * Multiplicative);
}

void UChopItCombatStatsComponent::ClearModifiers()
{
	Modifiers.Reset();
	OnStatsChanged.Broadcast();
}
