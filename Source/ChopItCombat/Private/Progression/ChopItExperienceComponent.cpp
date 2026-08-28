#include "Progression/ChopItExperienceComponent.h"

#include "Curves/CurveFloat.h"

UChopItExperienceComponent::UChopItExperienceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	ExperienceCurve = TSoftObjectPtr<UCurveFloat>(
		FSoftObjectPath(TEXT("/Game/ChopIt/Progression/Curves/Curve_XP_Levels.Curve_XP_Levels")));
}

int32 UChopItExperienceComponent::GetRequiredExperience() const
{
	const UCurveFloat* Curve = ExperienceCurve.LoadSynchronous();
	const float CurveValue = Curve ? Curve->GetFloatValue(static_cast<float>(Level)) : 10.0f + 5.0f * (Level - 1);
	return FMath::Max(1, FMath::RoundToInt(CurveValue));
}

void UChopItExperienceComponent::AddExperience(const int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	TotalExperience = FMath::Min<int64>(MAX_int64, TotalExperience + static_cast<int64>(Amount));
	CurrentExperience = static_cast<int32>(FMath::Min<int64>(MAX_int32, static_cast<int64>(CurrentExperience) + Amount));
	int32 Safety = 0;
	while (CurrentExperience >= GetRequiredExperience() && Safety++ < 100)
	{
		CurrentExperience -= GetRequiredExperience();
		++Level;
		++PendingLevelUps;
		OnLevelUpQueued.Broadcast(Level, PendingLevelUps);
	}
	OnExperienceChanged.Broadcast(Level, CurrentExperience, GetRequiredExperience(), PendingLevelUps);
}

bool UChopItExperienceComponent::ConsumePendingLevelUp()
{
	if (PendingLevelUps <= 0)
	{
		return false;
	}
	--PendingLevelUps;
	OnExperienceChanged.Broadcast(Level, CurrentExperience, GetRequiredExperience(), PendingLevelUps);
	return true;
}
