#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "ChopItTargetingSubsystem.generated.h"

class UChopItHealthComponent;

UCLASS()
class CHOPITCOMBAT_API UChopItTargetingSubsystem final : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterTarget(UChopItHealthComponent* Target);
	void UnregisterTarget(UChopItHealthComponent* Target);
	UChopItHealthComponent* FindNearestTarget(const FVector& Origin, float MaxRange, const AActor* IgnoredActor);
	TArray<UChopItHealthComponent*> FindTargetsInArc(
		const FVector& Origin,
		const FVector& Forward,
		float MaxRange,
		float ArcHalfAngleDegrees,
		int32 MaxTargets,
		const AActor* IgnoredActor);
	TArray<UChopItHealthComponent*> FindTargetsInRadius(
		const FVector& Origin,
		float MaxRange,
		int32 MaxTargets,
		const AActor* IgnoredActor);
	static bool IsLocationInsideArc(
		const FVector& Origin,
		const FVector& Forward,
		const FVector& Location,
		float MaxRange,
		float ArcHalfAngleDegrees);
	int32 GetRegisteredTargetCount() const;

private:
	TArray<TWeakObjectPtr<UChopItHealthComponent>> Targets;
};
