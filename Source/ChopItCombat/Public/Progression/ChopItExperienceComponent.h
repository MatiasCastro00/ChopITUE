#pragma once

#include "Components/ActorComponent.h"
#include "ChopItExperienceComponent.generated.h"

class UCurveFloat;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FChopItExperienceChanged,
	int32, Level,
	int32, CurrentExperience,
	int32, RequiredExperience,
	int32, PendingLevelUps);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChopItLevelUpQueued, int32, NewLevel, int32, PendingLevelUps);

/** Run-scoped integer XP ledger. It queues every crossed level without dropping simultaneous level-ups. */
UCLASS(ClassGroup = (ChopIt), BlueprintType, meta = (BlueprintSpawnableComponent))
class CHOPITCOMBAT_API UChopItExperienceComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItExperienceComponent();

	UFUNCTION(BlueprintCallable, Category = "ChopIt|Progression")
	void AddExperience(int32 Amount);

	bool ConsumePendingLevelUp();
	int32 GetRequiredExperience() const;
	int32 GetLevel() const { return Level; }
	int32 GetCurrentExperience() const { return CurrentExperience; }
	int64 GetTotalExperience() const { return TotalExperience; }
	int32 GetPendingLevelUps() const { return PendingLevelUps; }
	void SetExperienceCurve(UCurveFloat* Curve) { ExperienceCurve = Curve; }

	UPROPERTY(BlueprintAssignable, Category = "ChopIt|Progression")
	FChopItExperienceChanged OnExperienceChanged;

	UPROPERTY(BlueprintAssignable, Category = "ChopIt|Progression")
	FChopItLevelUpQueued OnLevelUpQueued;

private:
	UPROPERTY(EditDefaultsOnly, Category = "ChopIt|Progression")
	TSoftObjectPtr<UCurveFloat> ExperienceCurve;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Progression")
	int32 Level = 1;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Progression")
	int32 CurrentExperience = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Progression")
	int64 TotalExperience = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Progression")
	int32 PendingLevelUps = 0;
};
