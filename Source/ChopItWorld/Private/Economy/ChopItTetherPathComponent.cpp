#include "Economy/ChopItTetherPathComponent.h"

#include "ChopItCollision.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Economy/ChopItChainDefinition.h"
#include "Engine/World.h"

FVector FChopItWrapAnchor::GetWorldPosition() const
{
	return Component.IsValid()
		? Component->GetComponentTransform().TransformPosition(LocalPosition)
		: FallbackWorldPosition;
}

FVector FChopItWrapAnchor::GetWorldNormal() const
{
	return Component.IsValid()
		? Component->GetComponentTransform().TransformVectorNoScale(LocalNormal).GetSafeNormal()
		: FallbackWorldNormal.GetSafeNormal();
}

UChopItTetherPathComponent::UChopItTetherPathComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItTetherPathComponent::Configure(const UChopItChainDefinition* Definition)
{
	if (!Definition)
	{
		return;
	}
	SweepRadius = FMath::Max(1.0f, Definition->WrapSweepRadius);
	SurfaceOffset = FMath::Max(0.1f, Definition->WrapAnchorSurfaceOffset);
	MinimumAnchorSeparation = FMath::Max(1.0f, Definition->WrapMinimumAnchorSeparation);
	MaximumAnchors = FMath::Clamp(Definition->MaximumWrapAnchors, 1, 32);
	MaximumInsertionsPerFrame = FMath::Clamp(Definition->MaximumWrapInsertionsPerFrame, 1, 8);
	UnwrapFrames = FMath::Clamp(Definition->UnwrapConfirmationFrames, 1, 12);
	bDebugDraw = Definition->bDebugDrawTether;
}

void UChopItTetherPathComponent::InitializePath(
	const FVector& StartWorld,
	const FVector& EndWorld,
	AActor* InIgnoredEndActor)
{
	ResetPath();
	StartEndpoint = StartWorld;
	EndEndpoint = EndWorld;
	IgnoredEndActor = InIgnoredEndActor;
	RefreshRouteCache();
	TryInsertAnchors();
	RefreshRouteCache();
}

void UChopItTetherPathComponent::UpdatePath(const FVector& StartWorld, const FVector& EndWorld)
{
	StartEndpoint = StartWorld;
	EndEndpoint = EndWorld;
	RemoveInvalidAnchors();
	RefreshRouteCache();
	TryUnwrapAnchors();
	RefreshRouteCache();
	TryInsertAnchors();
	RefreshRouteCache();
	if (bDebugDraw)
	{
		DrawDebugRoute();
	}
}

void UChopItTetherPathComponent::ResetPath()
{
	Anchors.Reset();
	RoutePoints.Reset();
	RoutePointIds.Reset();
	IgnoredEndActor.Reset();
	RouteLength = 0.0f;
	NextStableId = 1;
	bAtAnchorCapacity = false;
}

float UChopItTetherPathComponent::CalculatePolylineLength(const TArray<FVector>& Points)
{
	float Length = 0.0f;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		Length += FVector::Distance(Points[Index - 1], Points[Index]);
	}
	return Length;
}

float UChopItTetherPathComponent::GetPrefixLengthBeforeFinalSpan() const
{
	float Length = 0.0f;
	for (int32 Index = 1; Index < RoutePoints.Num() - 1; ++Index)
	{
		Length += FVector::Distance(RoutePoints[Index - 1], RoutePoints[Index]);
	}
	return Length;
}

FVector UChopItTetherPathComponent::GetFinalGuidePoint() const
{
	return RoutePoints.Num() >= 2 ? RoutePoints[RoutePoints.Num() - 2] : StartEndpoint;
}

void UChopItTetherPathComponent::RefreshRouteCache()
{
	RoutePoints.Reset(Anchors.Num() + 2);
	RoutePointIds.Reset(Anchors.Num() + 2);
	RoutePoints.Add(StartEndpoint);
	RoutePointIds.Add(0);
	for (const FChopItWrapAnchor& Anchor : Anchors)
	{
		RoutePoints.Add(Anchor.GetWorldPosition());
		RoutePointIds.Add(Anchor.StableId);
	}
	RoutePoints.Add(EndEndpoint);
	RoutePointIds.Add(-1);
	RouteLength = CalculatePolylineLength(RoutePoints);
}

void UChopItTetherPathComponent::RemoveInvalidAnchors()
{
	for (int32 Index = Anchors.Num() - 1; Index >= 0; --Index)
	{
		if (!Anchors[Index].Component.IsValid())
		{
			Anchors.RemoveAt(Index);
		}
	}
}

void UChopItTetherPathComponent::TryUnwrapAnchors()
{
	// Remove at most one bend per frame. This avoids changing route topology twice
	// while an object is moving and makes the three-frame confirmation meaningful.
	for (int32 AnchorIndex = Anchors.Num() - 1; AnchorIndex >= 0; --AnchorIndex)
	{
		const FVector Previous = AnchorIndex == 0
			? StartEndpoint
			: Anchors[AnchorIndex - 1].GetWorldPosition();
		const FVector Next = AnchorIndex + 1 == Anchors.Num()
			? EndEndpoint
			: Anchors[AnchorIndex + 1].GetWorldPosition();
		const FVector Direction = (Next - Previous).GetSafeNormal();
		FHitResult Hit;
		const bool bBlocked = SweepSegment(
			Previous + Direction * MinimumAnchorSeparation * 0.25f,
			Next - Direction * MinimumAnchorSeparation * 0.25f,
			Hit);
		if (!bBlocked)
		{
			++Anchors[AnchorIndex].ClearFrames;
			if (Anchors[AnchorIndex].ClearFrames >= UnwrapFrames)
			{
				Anchors.RemoveAt(AnchorIndex);
				return;
			}
		}
		else
		{
			Anchors[AnchorIndex].ClearFrames = 0;
		}
	}
}

void UChopItTetherPathComponent::TryInsertAnchors()
{
	bAtAnchorCapacity = false;
	for (int32 Insertion = 0; Insertion < MaximumInsertionsPerFrame; ++Insertion)
	{
		RefreshRouteCache();
		bool bInserted = false;
		for (int32 SegmentIndex = 0; SegmentIndex < RoutePoints.Num() - 1; ++SegmentIndex)
		{
			const FVector Segment = RoutePoints[SegmentIndex + 1] - RoutePoints[SegmentIndex];
			const float SegmentLength = Segment.Size();
			if (SegmentLength <= MinimumAnchorSeparation)
			{
				continue;
			}
			const FVector Direction = Segment / SegmentLength;
			FHitResult Hit;
			if (!SweepSegment(
				RoutePoints[SegmentIndex] + Direction * MinimumAnchorSeparation * 0.2f,
				RoutePoints[SegmentIndex + 1] - Direction * MinimumAnchorSeparation * 0.2f,
				Hit))
			{
				continue;
			}

			UPrimitiveComponent* HitComponent = Hit.GetComponent();
			const FVector Candidate = Hit.ImpactPoint
				+ Hit.ImpactNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector)
				* (SweepRadius + SurfaceOffset);
			bool bRepresentedByNearbyAnchor = false;
			for (int32 NearbyIndex = FMath::Max(0, SegmentIndex - 1);
				NearbyIndex <= FMath::Min(Anchors.Num() - 1, SegmentIndex);
				++NearbyIndex)
			{
				FChopItWrapAnchor& Nearby = Anchors[NearbyIndex];
				if (Nearby.Component.Get() == HitComponent
					&& FVector::DistSquared(Nearby.GetWorldPosition(), Candidate)
						<= FMath::Square(MinimumAnchorSeparation))
				{
					const int32 StableId = Nearby.StableId;
					Nearby = MakeAnchor(Hit);
					Nearby.StableId = StableId;
					bRepresentedByNearbyAnchor = true;
					break;
				}
			}
			if (bRepresentedByNearbyAnchor)
			{
				// The sweep is touching the surface already represented by this
				// endpoint. Keep scanning later spans instead of inserting duplicates.
				continue;
			}
			if (Anchors.Num() >= MaximumAnchors)
			{
				bAtAnchorCapacity = true;
				return;
			}
			Anchors.Insert(MakeAnchor(Hit), SegmentIndex);
			bInserted = true;
			break;
		}
		if (!bInserted)
		{
			return;
		}
	}
}

bool UChopItTetherPathComponent::SweepSegment(
	const FVector& Start,
	const FVector& End,
	FHitResult& OutHit,
	const UPrimitiveComponent* IgnoredComponent) const
{
	UWorld* World = GetWorld();
	if (!World || FVector::DistSquared(Start, End) <= UE_SMALL_NUMBER)
	{
		return false;
	}
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ChopItTetherPathSweep), false, GetOwner());
	if (IgnoredEndActor.IsValid())
	{
		QueryParams.AddIgnoredActor(IgnoredEndActor.Get());
	}
	if (IgnoredComponent)
	{
		QueryParams.AddIgnoredComponent(IgnoredComponent);
	}
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ChopItCollisionChannels::Enemy, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ChopItCollisionChannels::Projectile, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ChopItCollisionChannels::Pickup, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ChopItCollisionChannels::DeliveryZone, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ChopItCollisionChannels::Chain, ECR_Ignore);
	return World->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		ChopItCollisionChannels::Chain,
		FCollisionShape::MakeSphere(SweepRadius),
		QueryParams,
		ResponseParams);
}

FChopItWrapAnchor UChopItTetherPathComponent::MakeAnchor(const FHitResult& Hit)
{
	FChopItWrapAnchor Anchor;
	Anchor.Component = Hit.GetComponent();
	Anchor.FallbackWorldNormal = Hit.ImpactNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	Anchor.FallbackWorldPosition = Hit.ImpactPoint
		+ Anchor.FallbackWorldNormal * (SweepRadius + SurfaceOffset);
	if (Anchor.Component.IsValid())
	{
		const FTransform& Transform = Anchor.Component->GetComponentTransform();
		Anchor.LocalPosition = Transform.InverseTransformPosition(Anchor.FallbackWorldPosition);
		Anchor.LocalNormal = Transform.InverseTransformVectorNoScale(Anchor.FallbackWorldNormal).GetSafeNormal();
	}
	Anchor.StableId = NextStableId++;
	return Anchor;
}

void UChopItTetherPathComponent::ApplyTensionToPhysicsProps(
	const float TensionAlpha,
	const float MaximumForce,
	const float MassScale) const
{
	if (TensionAlpha <= 0.0f || MaximumForce <= 0.0f)
	{
		return;
	}
	for (int32 Index = 0; Index < Anchors.Num(); ++Index)
	{
		UPrimitiveComponent* Component = Anchors[Index].Component.Get();
		if (!Component || !Component->IsSimulatingPhysics() || Component->GetOwner() == IgnoredEndActor.Get())
		{
			continue;
		}
		const FVector Position = Anchors[Index].GetWorldPosition();
		const FVector Previous = Index == 0 ? StartEndpoint : Anchors[Index - 1].GetWorldPosition();
		const FVector Next = Index + 1 == Anchors.Num() ? EndEndpoint : Anchors[Index + 1].GetWorldPosition();
		const FVector NetDirection = (Previous - Position).GetSafeNormal() + (Next - Position).GetSafeNormal();
		const float MassMultiplier = FMath::Clamp(Component->GetMass() / 50.0f, 0.25f, 4.0f);
		FVector Force = NetDirection * MaximumForce * FMath::Clamp(TensionAlpha, 0.0f, 1.0f)
			* FMath::Max(0.0f, MassScale) * MassMultiplier;
		Force = Force.GetClampedToMaxSize(MaximumForce);
		Component->AddForceAtLocation(Force, Position);
	}
}

void UChopItTetherPathComponent::DrawDebugRoute() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (int32 Index = 1; Index < RoutePoints.Num(); ++Index)
	{
		DrawDebugLine(World, RoutePoints[Index - 1], RoutePoints[Index], FColor::Cyan, false, 0.0f, 0, 2.0f);
	}
	for (const FChopItWrapAnchor& Anchor : Anchors)
	{
		const FVector Position = Anchor.GetWorldPosition();
		DrawDebugSphere(World, Position, SweepRadius, 8, FColor::Yellow, false, 0.0f, 0, 1.5f);
		DrawDebugDirectionalArrow(World, Position, Position + Anchor.GetWorldNormal() * 30.0f, 8.0f, FColor::Green, false, 0.0f);
	}
}
