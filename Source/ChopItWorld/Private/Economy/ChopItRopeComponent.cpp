#include "Economy/ChopItRopeComponent.h"

#include "ChopItCollision.h"
#include "CollisionQueryParams.h"
#include "Economy/ChopItChainDefinition.h"
#include "Engine/World.h"

UChopItRopeComponent::UChopItRopeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItRopeComponent::Configure(const UChopItChainDefinition* Definition)
{
	if (!Definition)
	{
		return;
	}
	const int32 MaximumLinks = FMath::Clamp(Definition->ChainLinkCount, 3, 64);
	const int32 SegmentsPerLink = FMath::Clamp(Definition->CableSegmentsPerLink, 2, 8);
	TargetParticleSpacing = Definition->MaxChainLength / FMath::Max(1, MaximumLinks * SegmentsPerLink);
	CollisionRadius = FMath::Max(FMath::Max(1.0f, Definition->CableParticleDiameter * 0.5f), TargetParticleSpacing * 0.55f);
	CollisionSkin = FMath::Max(0.1f, Definition->CableCollisionSkin);
	SolverIterations = FMath::Clamp(Definition->CableSolverIterations, 1, 64);
	CollisionIterations = FMath::Clamp(Definition->CableCollisionIterations, 1, 8);
	SubstepTime = FMath::Clamp(Definition->CableSubstepTime, 0.0025f, 0.033f);
	MaximumSubsteps = FMath::Clamp(Definition->CableMaximumSubsteps, 1, 16);
	GravityScale = FMath::Max(0.0f, Definition->CableGravityScale)
		* (FMath::Max(0.01f, Definition->ChainLinkWeight) / 1.25f);
	VelocityDamping = FMath::Clamp(Definition->CableVelocityDamping, 0.0f, 0.25f);
	ConstraintVelocityDamping = FMath::Clamp(Definition->CableConstraintVelocityDamping, 0.0f, 1.0f);
	ConstraintStiffness = FMath::Clamp(Definition->CableXPBDStiffness, 0.5f, 1.0f);
	GroundFriction = FMath::Clamp(Definition->CableGroundFriction, 0.0f, 1.0f);
	ObstacleFriction = FMath::Clamp(Definition->CableCollisionFriction, 0.0f, 1.0f);
	MinimumParticlesPerSpan = FMath::Clamp(Definition->MinimumVisualParticlesPerSpan, 3, 64);
	bWorldCollision = Definition->bCableWorldCollision;
}

void UChopItRopeComponent::InitializeRope(
	const FVector& StartWorld,
	const FVector& EndWorld,
	const float InRopeLength,
	AActor* InIgnoredEndActor)
{
	IgnoredEndActor = InIgnoredEndActor;
	bInitialized = true;
	SetRoutePath(TArray<FVector>{StartWorld, EndWorld}, TArray<int32>{0, -1}, InRopeLength);
	AccumulatedTime = 0.0f;
}

void UChopItRopeComponent::SetRoutePath(
	const TArray<FVector>& InRoutePoints,
	const TArray<int32>& InRoutePointIds,
	const float InRopeLength)
{
	if (InRoutePoints.Num() < 2 || InRoutePointIds.Num() != InRoutePoints.Num())
	{
		return;
	}
	const bool bTopologyChanged = RoutePointIds != InRoutePointIds;
	RoutePoints = InRoutePoints;
	RoutePointIds = InRoutePointIds;
	RopeLength = FMath::Max(1.0f, InRopeLength);
	bInitialized = true;
	UpdateSpanLayout(bTopologyChanged || Spans.Num() != RoutePoints.Num() - 1);
	RefreshFlattenedPositions();
}

void UChopItRopeComponent::SetEndpoints(const FVector& StartWorld, const FVector& EndWorld)
{
	if (RoutePoints.Num() != 2)
	{
		SetRoutePath(TArray<FVector>{StartWorld, EndWorld}, TArray<int32>{0, -1}, RopeLength);
		return;
	}
	RoutePoints[0] = StartWorld;
	RoutePoints[1] = EndWorld;
	UpdateSpanLayout(false);
	RefreshFlattenedPositions();
}

void UChopItRopeComponent::SetRopeLength(const float InRopeLength)
{
	RopeLength = FMath::Max(1.0f, InRopeLength);
	if (bInitialized)
	{
		UpdateSpanLayout(false);
		RefreshFlattenedPositions();
	}
}

void UChopItRopeComponent::UpdateSpanLayout(const bool bTopologyChanged)
{
	if (bTopologyChanged)
	{
		RebuildSpansPreservingState();
	}
	if (Spans.Num() != RoutePoints.Num() - 1)
	{
		return;
	}

	TArray<float> DirectLengths;
	DirectLengths.SetNumUninitialized(Spans.Num());
	float DirectTotal = 0.0f;
	for (int32 Index = 0; Index < Spans.Num(); ++Index)
	{
		DirectLengths[Index] = FVector::Distance(RoutePoints[Index], RoutePoints[Index + 1]);
		DirectTotal += DirectLengths[Index];
	}
	const float Slack = FMath::Max(0.0f, RopeLength - DirectTotal);
	for (int32 Index = 0; Index < Spans.Num(); ++Index)
	{
		FChopItVisualRopeSpan& Span = Spans[Index];
		Span.StartId = RoutePointIds[Index];
		Span.EndId = RoutePointIds[Index + 1];
		const float Weight = DirectTotal > UE_SMALL_NUMBER
			? DirectLengths[Index] / DirectTotal
			: 1.0f / FMath::Max(1, Spans.Num());
		Span.RestLength = FMath::Max(1.0f, DirectLengths[Index] + Slack * Weight);
		const int32 DesiredCount = FMath::Clamp(
			FMath::CeilToInt(Span.RestLength / FMath::Max(1.0f, TargetParticleSpacing)) + 1,
			MinimumParticlesPerSpan,
			129);
		ResizeSpan(Span, DesiredCount);
		PinSpan(Span, Index);
		if (bTopologyChanged)
		{
			// Splitting or merging keeps the old curve and velocity, then projects
			// that preserved shape into the new span budgets immediately. Without
			// this topology-only settle pass, tiny per-span errors could add up
			// visually when several obstacles were introduced in one frame.
			for (int32 Iteration = 0; Iteration < 256; ++Iteration)
			{
				SolveSpanConstraints(Span, (Iteration & 1) != 0);
				PinSpan(Span, Index);
			}
			ProjectSpanToRestLength(Span);
		}
	}
}

void UChopItRopeComponent::RebuildSpansPreservingState()
{
	TArray<FVector> OldPositions;
	TArray<FVector> OldPrevious;
	BuildFlattenedPolyline(false, OldPositions);
	BuildFlattenedPolyline(true, OldPrevious);

	TArray<FChopItVisualRopeSpan> NewSpans;
	NewSpans.SetNum(RoutePoints.Num() - 1);
	for (int32 SpanIndex = 0; SpanIndex < NewSpans.Num(); ++SpanIndex)
	{
		FChopItVisualRopeSpan& Span = NewSpans[SpanIndex];
		Span.StartId = RoutePointIds[SpanIndex];
		Span.EndId = RoutePointIds[SpanIndex + 1];
		const float DirectLength = FVector::Distance(RoutePoints[SpanIndex], RoutePoints[SpanIndex + 1]);
		const int32 Count = FMath::Clamp(
			FMath::CeilToInt(FMath::Max(DirectLength, 1.0f) / FMath::Max(1.0f, TargetParticleSpacing)) + 1,
			MinimumParticlesPerSpan,
			129);
		Span.Positions.SetNumUninitialized(Count);
		Span.PreviousPositions.SetNumUninitialized(Count);

		const bool bCanPreserve = OldPositions.Num() >= 2 && OldPrevious.Num() == OldPositions.Num();
		const float StartDistance = bCanPreserve ? FindNearestPathDistance(OldPositions, RoutePoints[SpanIndex]) : 0.0f;
		const float EndDistance = bCanPreserve ? FindNearestPathDistance(OldPositions, RoutePoints[SpanIndex + 1]) : 0.0f;
		for (int32 ParticleIndex = 0; ParticleIndex < Count; ++ParticleIndex)
		{
			const float Alpha = static_cast<float>(ParticleIndex) / FMath::Max(1, Count - 1);
			if (bCanPreserve && EndDistance > StartDistance + UE_SMALL_NUMBER)
			{
				const float SampleDistance = FMath::Lerp(StartDistance, EndDistance, Alpha);
				Span.Positions[ParticleIndex] = SamplePath(OldPositions, SampleDistance);
				Span.PreviousPositions[ParticleIndex] = SamplePath(OldPrevious, SampleDistance);
			}
			else
			{
				const FVector Position = FMath::Lerp(RoutePoints[SpanIndex], RoutePoints[SpanIndex + 1], Alpha);
				Span.Positions[ParticleIndex] = Position;
				Span.PreviousPositions[ParticleIndex] = Position;
			}
		}
	}
	Spans = MoveTemp(NewSpans);
}

void UChopItRopeComponent::ResizeSpan(FChopItVisualRopeSpan& Span, const int32 DesiredCount)
{
	if (Span.Positions.Num() < 2 || Span.PreviousPositions.Num() != Span.Positions.Num())
	{
		Span.Positions.SetNumUninitialized(DesiredCount);
		Span.PreviousPositions.SetNumUninitialized(DesiredCount);
		const int32 SpanIndex = static_cast<int32>(&Span - Spans.GetData());
		for (int32 Index = 0; Index < DesiredCount; ++Index)
		{
			const float Alpha = static_cast<float>(Index) / FMath::Max(1, DesiredCount - 1);
			const FVector Position = FMath::Lerp(RoutePoints[SpanIndex], RoutePoints[SpanIndex + 1], Alpha);
			Span.Positions[Index] = Position;
			Span.PreviousPositions[Index] = Position;
		}
		return;
	}
	while (Span.Positions.Num() < DesiredCount)
	{
		const FVector Position = FMath::Lerp(Span.Positions[0], Span.Positions[1], 0.35f);
		Span.Positions.Insert(Position, 1);
		Span.PreviousPositions.Insert(Position, 1);
	}
	while (Span.Positions.Num() > DesiredCount && Span.Positions.Num() > MinimumParticlesPerSpan)
	{
		Span.Positions.RemoveAt(1, 1, EAllowShrinking::No);
		Span.PreviousPositions.RemoveAt(1, 1, EAllowShrinking::No);
	}
}

void UChopItRopeComponent::Simulate(const float DeltaSeconds)
{
	if (!bInitialized || Spans.IsEmpty() || DeltaSeconds <= 0.0f)
	{
		return;
	}
	AccumulatedTime += FMath::Min(DeltaSeconds, SubstepTime * MaximumSubsteps);
	for (int32 Completed = 0; AccumulatedTime >= SubstepTime && Completed < MaximumSubsteps; ++Completed)
	{
		SimulateSubstep(SubstepTime);
		AccumulatedTime -= SubstepTime;
	}
	RefreshFlattenedPositions();
}

void UChopItRopeComponent::SimulateSubstep(const float StepSeconds)
{
	const FVector Gravity(0.0f, 0.0f, GetWorld() ? GetWorld()->GetGravityZ() * GravityScale : -980.0f * GravityScale);
	for (int32 SpanIndex = 0; SpanIndex < Spans.Num(); ++SpanIndex)
	{
		FChopItVisualRopeSpan& Span = Spans[SpanIndex];
		TArray<FVector> SweepStarts = Span.Positions;
		for (int32 Index = 1; Index < Span.Positions.Num() - 1; ++Index)
		{
			const FVector Current = Span.Positions[Index];
			const FVector Velocity = (Current - Span.PreviousPositions[Index]) * (1.0f - VelocityDamping);
			Span.PreviousPositions[Index] = Current;
			Span.Positions[Index] = Current + Velocity + Gravity * FMath::Square(StepSeconds);
		}
		const int32 EffectiveIterations = FMath::Clamp(
			FMath::Max(SolverIterations, (Span.Positions.Num() - 1) * 4), 1, 128);
		for (int32 Iteration = 0; Iteration < EffectiveIterations; ++Iteration)
		{
			SolveSpanConstraints(Span, (Iteration & 1) != 0);
			if (bWorldCollision && Iteration < CollisionIterations)
			{
				ResolveSpanCollisions(Span, SweepStarts);
				SweepStarts = Span.Positions;
			}
			PinSpan(Span, SpanIndex);
		}
		ProjectSpanToRestLength(Span);
	}
}

void UChopItRopeComponent::SolveSpanConstraints(FChopItVisualRopeSpan& Span, const bool bReverseOrder)
{
	const float SegmentLength = Span.RestLength / FMath::Max(1, Span.Positions.Num() - 1);
	const auto Solve = [this, &Span, SegmentLength](const int32 Index)
	{
		FVector& First = Span.Positions[Index];
		FVector& Second = Span.Positions[Index + 1];
		FVector& PreviousFirst = Span.PreviousPositions[Index];
		FVector& PreviousSecond = Span.PreviousPositions[Index + 1];
		const FVector Delta = Second - First;
		const float Distance = Delta.Size();
		if (Distance <= SegmentLength || Distance <= UE_SMALL_NUMBER)
		{
			return;
		}
		const FVector Correction = Delta * ((Distance - SegmentLength) / Distance) * ConstraintStiffness;
		if (Index == 0)
		{
			Second -= Correction;
			PreviousSecond -= Correction * ConstraintVelocityDamping;
		}
		else if (Index + 1 == Span.Positions.Num() - 1)
		{
			First += Correction;
			PreviousFirst += Correction * ConstraintVelocityDamping;
		}
		else
		{
			First += Correction * 0.5f;
			Second -= Correction * 0.5f;
			PreviousFirst += Correction * (0.5f * ConstraintVelocityDamping);
			PreviousSecond -= Correction * (0.5f * ConstraintVelocityDamping);
		}
	};
	if (bReverseOrder)
	{
		for (int32 Index = Span.Positions.Num() - 2; Index >= 0; --Index)
		{
			Solve(Index);
		}
	}
	else
	{
		for (int32 Index = 0; Index < Span.Positions.Num() - 1; ++Index)
		{
			Solve(Index);
		}
	}
}

void UChopItRopeComponent::ProjectSpanToRestLength(FChopItVisualRopeSpan& Span)
{
	if (Span.Positions.Num() < 3)
	{
		return;
	}
	float CurrentLength = 0.0f;
	for (int32 Index = 1; Index < Span.Positions.Num(); ++Index)
	{
		CurrentLength += FVector::Distance(Span.Positions[Index - 1], Span.Positions[Index]);
	}
	if (CurrentLength <= Span.RestLength + 0.05f)
	{
		return;
	}

	const TArray<FVector> BeforeProjection = Span.Positions;
	const FVector StartTarget = Span.Positions[0];
	const FVector EndTarget = Span.Positions.Last();
	const float SegmentLength = Span.RestLength / FMath::Max(1, Span.Positions.Num() - 1);
	for (int32 Iteration = 0; Iteration < 96; ++Iteration)
	{
		Span.Positions.Last() = EndTarget;
		for (int32 Index = Span.Positions.Num() - 2; Index >= 0; --Index)
		{
			const FVector Direction = (Span.Positions[Index] - Span.Positions[Index + 1])
				.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
			Span.Positions[Index] = Span.Positions[Index + 1] + Direction * SegmentLength;
		}
		Span.Positions[0] = StartTarget;
		for (int32 Index = 1; Index < Span.Positions.Num(); ++Index)
		{
			const FVector Direction = (Span.Positions[Index] - Span.Positions[Index - 1])
				.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
			Span.Positions[Index] = Span.Positions[Index - 1] + Direction * SegmentLength;
		}
		if (FVector::DistSquared(Span.Positions.Last(), EndTarget) < 0.0001f)
		{
			break;
		}
	}
	Span.Positions.Last() = EndTarget;
	for (int32 Index = 1; Index < Span.Positions.Num() - 1; ++Index)
	{
		Span.PreviousPositions[Index] += (Span.Positions[Index] - BeforeProjection[Index])
			* ConstraintVelocityDamping;
	}
}

void UChopItRopeComponent::ResolveSpanCollisions(FChopItVisualRopeSpan& Span, const TArray<FVector>& SweepStarts)
{
	UWorld* World = GetWorld();
	if (!World || SweepStarts.Num() != Span.Positions.Num())
	{
		return;
	}
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ChopItVisualRopeSweep), false, GetOwner());
	if (IgnoredEndActor.IsValid())
	{
		QueryParams.AddIgnoredActor(IgnoredEndActor.Get());
	}
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ChopItCollisionChannels::Enemy, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ChopItCollisionChannels::Projectile, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ChopItCollisionChannels::Pickup, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ChopItCollisionChannels::DeliveryZone, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ChopItCollisionChannels::Chain, ECR_Ignore);
	for (int32 Index = 1; Index < Span.Positions.Num() - 1; ++Index)
	{
		FHitResult Hit;
		if (!World->SweepSingleByChannel(Hit, SweepStarts[Index], Span.Positions[Index], FQuat::Identity,
			ChopItCollisionChannels::Chain, FCollisionShape::MakeSphere(CollisionRadius), QueryParams, ResponseParams))
		{
			continue;
		}
		const FVector Normal = Hit.ImpactNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		Span.Positions[Index] = Hit.bStartPenetrating
			? SweepStarts[Index] + Normal * (Hit.PenetrationDepth + CollisionSkin)
			: Hit.Location + Normal * CollisionSkin;
		FVector Velocity = Span.Positions[Index] - Span.PreviousPositions[Index];
		const float NormalSpeed = FVector::DotProduct(Velocity, Normal);
		if (NormalSpeed < 0.0f)
		{
			Velocity -= Normal * NormalSpeed;
		}
		const float Friction = Normal.Z > 0.65f ? GroundFriction : ObstacleFriction;
		const FVector NormalVelocity = Normal * FVector::DotProduct(Velocity, Normal);
		const FVector TangentVelocity = (Velocity - NormalVelocity) * (1.0f - Friction);
		Span.PreviousPositions[Index] = Span.Positions[Index] - NormalVelocity - TangentVelocity;
	}
}

void UChopItRopeComponent::PinSpan(FChopItVisualRopeSpan& Span, const int32 SpanIndex)
{
	if (!RoutePoints.IsValidIndex(SpanIndex + 1) || Span.Positions.Num() < 2)
	{
		return;
	}
	Span.Positions[0] = RoutePoints[SpanIndex];
	Span.PreviousPositions[0] = RoutePoints[SpanIndex];
	Span.Positions.Last() = RoutePoints[SpanIndex + 1];
	Span.PreviousPositions.Last() = RoutePoints[SpanIndex + 1];
}

void UChopItRopeComponent::RefreshFlattenedPositions()
{
	BuildFlattenedPolyline(false, FlattenedPositions);
}

void UChopItRopeComponent::BuildFlattenedPolyline(const bool bPrevious, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();
	for (int32 SpanIndex = 0; SpanIndex < Spans.Num(); ++SpanIndex)
	{
		const TArray<FVector>& Source = bPrevious ? Spans[SpanIndex].PreviousPositions : Spans[SpanIndex].Positions;
		for (int32 Index = SpanIndex == 0 ? 0 : 1; Index < Source.Num(); ++Index)
		{
			OutPoints.Add(Source[Index]);
		}
	}
}

float UChopItRopeComponent::FindNearestPathDistance(const TArray<FVector>& Points, const FVector& Point)
{
	float BestSquared = TNumericLimits<float>::Max();
	float BestPathDistance = 0.0f;
	float Traversed = 0.0f;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		const FVector Segment = Points[Index] - Points[Index - 1];
		const float SegmentSquared = Segment.SizeSquared();
		const float SegmentLength = FMath::Sqrt(SegmentSquared);
		const float Alpha = SegmentSquared > UE_SMALL_NUMBER
			? FMath::Clamp(FVector::DotProduct(Point - Points[Index - 1], Segment) / SegmentSquared, 0.0f, 1.0f)
			: 0.0f;
		const float Squared = FVector::DistSquared(Points[Index - 1] + Segment * Alpha, Point);
		if (Squared < BestSquared)
		{
			BestSquared = Squared;
			BestPathDistance = Traversed + SegmentLength * Alpha;
		}
		Traversed += SegmentLength;
	}
	return BestPathDistance;
}

FVector UChopItRopeComponent::SamplePath(const TArray<FVector>& Points, const float Distance)
{
	float Traversed = 0.0f;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		const float SegmentLength = FVector::Distance(Points[Index - 1], Points[Index]);
		if (Traversed + SegmentLength >= Distance && SegmentLength > UE_SMALL_NUMBER)
		{
			return FMath::Lerp(Points[Index - 1], Points[Index],
				FMath::Clamp((Distance - Traversed) / SegmentLength, 0.0f, 1.0f));
		}
		Traversed += SegmentLength;
	}
	return Points.IsEmpty() ? FVector::ZeroVector : Points.Last();
}

float UChopItRopeComponent::GetSimulatedPathLength() const
{
	float Length = 0.0f;
	for (int32 Index = 1; Index < FlattenedPositions.Num(); ++Index)
	{
		Length += FVector::Distance(FlattenedPositions[Index - 1], FlattenedPositions[Index]);
	}
	return Length;
}

void UChopItRopeComponent::ResetRope()
{
	Spans.Reset();
	RoutePoints.Reset();
	RoutePointIds.Reset();
	FlattenedPositions.Reset();
	IgnoredEndActor.Reset();
	AccumulatedTime = 0.0f;
	bInitialized = false;
}
