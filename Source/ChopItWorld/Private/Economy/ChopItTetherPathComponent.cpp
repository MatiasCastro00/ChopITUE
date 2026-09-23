#include "Economy/ChopItTetherPathComponent.h"
#include "Economy/ChopItRopeComponent.h"
#include "Economy/ChopItChainDefinition.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"

FVector FChopItWrapAnchor::GetWorldPosition() const
{
	return Component.IsValid() ? Component->GetComponentTransform().TransformPosition(LocalPosition) : FallbackWorldPosition;
}
FVector FChopItWrapAnchor::GetWorldNormal() const
{
	return Component.IsValid() ? Component->GetComponentTransform().TransformVectorNoScale(LocalNormal).GetSafeNormal() : FallbackWorldNormal;
}
UChopItTetherPathComponent::UChopItTetherPathComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}
void UChopItTetherPathComponent::Configure(const UChopItChainDefinition* Definition)
{
	bDebugDraw = Definition && Definition->bDebugDrawTether;
}
void UChopItTetherPathComponent::SetSource(UChopItRopeComponent* InSource)
{
	Source = InSource;
	Synchronize();
}
void UChopItTetherPathComponent::Synchronize()
{
	RoutePoints.Reset(); RoutePointIds.Reset(); Anchors.Reset();
	if (!Source.IsValid() || !Source->IsInitialized()) return;
	RoutePoints = Source->GetParticleLocations();
	for (int32 I = 0; I < RoutePoints.Num(); ++I) RoutePointIds.Add(I);
	for (const FChopItRopeContact& C : Source->GetContacts())
	{
		if (!C.Component.IsValid()) continue;
		FChopItWrapAnchor& A = Anchors.AddDefaulted_GetRef();
		A.Component = C.Component;
		A.FallbackWorldPosition = C.Position;
		A.FallbackWorldNormal = C.Normal;
		A.LocalPosition = C.Component->GetComponentTransform().InverseTransformPosition(C.Position);
		A.LocalNormal = C.Component->GetComponentTransform().InverseTransformVectorNoScale(C.Normal);
		A.StableId = C.Segment;
	}
	if (bDebugDraw && GetWorld())
	{
		for (int32 I = 1; I < RoutePoints.Num(); ++I)
			DrawDebugLine(GetWorld(), RoutePoints[I - 1], RoutePoints[I], FColor::Cyan, false, 0, 0, 2);
		for (const FChopItWrapAnchor& A : Anchors)
			DrawDebugDirectionalArrow(GetWorld(), A.GetWorldPosition(), A.GetWorldPosition() + A.GetWorldNormal() * 25, 5, FColor::Yellow, false, 0);
	}
}
void UChopItTetherPathComponent::ResetPath()
{
	Source.Reset(); RoutePoints.Reset(); RoutePointIds.Reset(); Anchors.Reset();
}
float UChopItTetherPathComponent::CalculatePolylineLength(const TArray<FVector>& Points)
{
	float Length = 0;
	for (int32 I = 1; I < Points.Num(); ++I) Length += FVector::Distance(Points[I - 1], Points[I]);
	return Length;
}
float UChopItTetherPathComponent::GetRouteLength() const { return CalculatePolylineLength(RoutePoints); }
float UChopItTetherPathComponent::GetPrefixLengthBeforeFinalSpan() const
{
	return RoutePoints.Num() > 1 ? GetRouteLength() - FVector::Distance(RoutePoints.Last(), RoutePoints[RoutePoints.Num() - 2]) : 0;
}
FVector UChopItTetherPathComponent::GetFinalGuidePoint() const
{
	return RoutePoints.Num() > 1 ? RoutePoints[RoutePoints.Num() - 2] : FVector::ZeroVector;
}
