#include "Targeting/ChopItTargetingSubsystem.h"

#include "Combat/ChopItHealthComponent.h"

void UChopItTargetingSubsystem::RegisterTarget(UChopItHealthComponent* Target)
{
	if (IsValid(Target))
	{
		Targets.AddUnique(Target);
	}
}

void UChopItTargetingSubsystem::UnregisterTarget(UChopItHealthComponent* Target)
{
	Targets.Remove(Target);
}

UChopItHealthComponent* UChopItTargetingSubsystem::FindNearestTarget(
	const FVector& Origin,
	const float MaxRange,
	const AActor* IgnoredActor)
{
	Targets.RemoveAllSwap([](const TWeakObjectPtr<UChopItHealthComponent>& Target)
	{
		return !Target.IsValid();
	});

	UChopItHealthComponent* Nearest = nullptr;
	float NearestDistanceSquared = FMath::Square(FMath::Max(0.0f, MaxRange));
	for (const TWeakObjectPtr<UChopItHealthComponent>& TargetPtr : Targets)
	{
		UChopItHealthComponent* Target = TargetPtr.Get();
		AActor* TargetOwner = Target ? Target->GetOwner() : nullptr;
		if (!Target || !Target->IsAlive() || !IsValid(TargetOwner) || TargetOwner == IgnoredActor)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(Origin, TargetOwner->GetActorLocation());
		if (DistanceSquared <= NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			Nearest = Target;
		}
	}
	return Nearest;
}

TArray<UChopItHealthComponent*> UChopItTargetingSubsystem::FindTargetsInArc(
	const FVector& Origin,
	const FVector& Forward,
	const float MaxRange,
	const float ArcHalfAngleDegrees,
	const int32 MaxTargets,
	const AActor* IgnoredActor)
{
	Targets.RemoveAllSwap([](const TWeakObjectPtr<UChopItHealthComponent>& Target)
	{
		return !Target.IsValid();
	});

	struct FArcCandidate
	{
		UChopItHealthComponent* Health = nullptr;
		double DistanceSquared = 0.0;
	};

	TArray<FArcCandidate> Candidates;
	Candidates.Reserve(Targets.Num());
	for (const TWeakObjectPtr<UChopItHealthComponent>& TargetPtr : Targets)
	{
		UChopItHealthComponent* Target = TargetPtr.Get();
		AActor* TargetOwner = Target ? Target->GetOwner() : nullptr;
		if (!Target || !Target->IsAlive() || !IsValid(TargetOwner) || TargetOwner == IgnoredActor)
		{
			continue;
		}

		if (IsLocationInsideArc(Origin, Forward, TargetOwner->GetActorLocation(), MaxRange, ArcHalfAngleDegrees))
		{
			Candidates.Add({ Target, FVector::DistSquared2D(Origin, TargetOwner->GetActorLocation()) });
		}
	}

	Candidates.Sort([](const FArcCandidate& Left, const FArcCandidate& Right)
	{
		return Left.DistanceSquared < Right.DistanceSquared;
	});

	TArray<UChopItHealthComponent*> Result;
	const int32 ResultCount = FMath::Min(FMath::Max(0, MaxTargets), Candidates.Num());
	Result.Reserve(ResultCount);
	for (int32 Index = 0; Index < ResultCount; ++Index)
	{
		Result.Add(Candidates[Index].Health);
	}
	return Result;
}

TArray<UChopItHealthComponent*> UChopItTargetingSubsystem::FindTargetsInRadius(
	const FVector& Origin,
	const float MaxRange,
	const int32 MaxTargets,
	const AActor* IgnoredActor)
{
	Targets.RemoveAllSwap([](const TWeakObjectPtr<UChopItHealthComponent>& Target)
	{
		return !Target.IsValid();
	});

	struct FRadiusCandidate
	{
		UChopItHealthComponent* Health = nullptr;
		double DistanceSquared = 0.0;
	};
	TArray<FRadiusCandidate> Candidates;
	const float MaxDistanceSquared = FMath::Square(FMath::Max(0.0f, MaxRange));
	for (const TWeakObjectPtr<UChopItHealthComponent>& TargetPtr : Targets)
	{
		UChopItHealthComponent* Target = TargetPtr.Get();
		AActor* TargetOwner = Target ? Target->GetOwner() : nullptr;
		if (!Target || !Target->IsAlive() || !IsValid(TargetOwner) || TargetOwner == IgnoredActor)
		{
			continue;
		}
		const double DistanceSquared = FVector::DistSquared2D(Origin, TargetOwner->GetActorLocation());
		if (DistanceSquared <= MaxDistanceSquared)
		{
			Candidates.Add({ Target, DistanceSquared });
		}
	}
	Candidates.Sort([](const FRadiusCandidate& Left, const FRadiusCandidate& Right)
	{
		return Left.DistanceSquared < Right.DistanceSquared;
	});
	TArray<UChopItHealthComponent*> Result;
	const int32 ResultCount = FMath::Min(FMath::Max(0, MaxTargets), Candidates.Num());
	Result.Reserve(ResultCount);
	for (int32 Index = 0; Index < ResultCount; ++Index)
	{
		Result.Add(Candidates[Index].Health);
	}
	return Result;
}

bool UChopItTargetingSubsystem::IsLocationInsideArc(
	const FVector& Origin,
	const FVector& Forward,
	const FVector& Location,
	const float MaxRange,
	const float ArcHalfAngleDegrees)
{
	FVector FlatForward(Forward.X, Forward.Y, 0.0f);
	FVector ToLocation(Location.X - Origin.X, Location.Y - Origin.Y, 0.0f);
	const float DistanceSquared = ToLocation.SizeSquared();
	if (!FlatForward.Normalize() || DistanceSquared <= UE_SMALL_NUMBER || DistanceSquared > FMath::Square(FMath::Max(0.0f, MaxRange)))
	{
		return false;
	}

	ToLocation.Normalize();
	const float ClampedHalfAngle = FMath::Clamp(ArcHalfAngleDegrees, 0.0f, 180.0f);
	return FVector::DotProduct(FlatForward, ToLocation) >= FMath::Cos(FMath::DegreesToRadians(ClampedHalfAngle));
}

int32 UChopItTargetingSubsystem::GetRegisteredTargetCount() const
{
	int32 ValidTargetCount = 0;
	for (const TWeakObjectPtr<UChopItHealthComponent>& Target : Targets)
	{
		ValidTargetCount += Target.IsValid() ? 1 : 0;
	}
	return ValidTargetCount;
}
