#pragma once

#include "Components/SceneComponent.h"
#include "ChopItRopeComponent.generated.h"

class UChopItChainDefinition;

struct FChopItVisualRopeSpan
{
	int32 StartId = INDEX_NONE;
	int32 EndId = INDEX_NONE;
	float RestLength = 1.0f;
	TArray<FVector> Positions;
	TArray<FVector> PreviousPositions;
};

/** XPBD-style presentation rope. Gameplay authority lives in the tether path. */
UCLASS(ClassGroup = (ChopIt), meta = (BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItRopeComponent final : public USceneComponent
{
	GENERATED_BODY()

public:
	UChopItRopeComponent();
	void Configure(const UChopItChainDefinition* Definition);
	void InitializeRope(const FVector& StartWorld, const FVector& EndWorld, float InRopeLength, AActor* InIgnoredEndActor);
	void SetRoutePath(const TArray<FVector>& InRoutePoints, const TArray<int32>& InRoutePointIds, float InRopeLength);
	void SetEndpoints(const FVector& StartWorld, const FVector& EndWorld);
	void SetRopeLength(float InRopeLength);
	void Simulate(float DeltaSeconds);
	void ResetRope();

	const TArray<FVector>& GetParticleLocations() const { return FlattenedPositions; }
	float GetRopeLength() const { return RopeLength; }
	float GetSimulatedPathLength() const;
	int32 GetSpanCount() const { return Spans.Num(); }
	bool IsInitialized() const { return bInitialized; }

private:
	void UpdateSpanLayout(bool bTopologyChanged);
	void RebuildSpansPreservingState();
	void ResizeSpan(FChopItVisualRopeSpan& Span, int32 DesiredCount);
	void SimulateSubstep(float StepSeconds);
	void SolveSpanConstraints(FChopItVisualRopeSpan& Span, bool bReverseOrder);
	void ProjectSpanToRestLength(FChopItVisualRopeSpan& Span);
	void ResolveSpanCollisions(FChopItVisualRopeSpan& Span, const TArray<FVector>& SweepStarts);
	void PinSpan(FChopItVisualRopeSpan& Span, int32 SpanIndex);
	void RefreshFlattenedPositions();
	void BuildFlattenedPolyline(bool bPrevious, TArray<FVector>& OutPoints) const;
	static float FindNearestPathDistance(const TArray<FVector>& Points, const FVector& Point);
	static FVector SamplePath(const TArray<FVector>& Points, float Distance);

	TArray<FChopItVisualRopeSpan> Spans;
	TArray<FVector> RoutePoints;
	TArray<int32> RoutePointIds;
	TArray<FVector> FlattenedPositions;
	TWeakObjectPtr<AActor> IgnoredEndActor;
	float RopeLength = 100.0f;
	float TargetParticleSpacing = 10.0f;
	float CollisionRadius = 5.0f;
	float CollisionSkin = 1.0f;
	float SubstepTime = 1.0f / 120.0f;
	float GravityScale = 1.0f;
	float VelocityDamping = 0.02f;
	float ConstraintVelocityDamping = 0.9f;
	float ConstraintStiffness = 0.985f;
	float GroundFriction = 0.0f;
	float ObstacleFriction = 0.2f;
	float AccumulatedTime = 0.0f;
	int32 SolverIterations = 32;
	int32 CollisionIterations = 3;
	int32 MaximumSubsteps = 8;
	int32 MinimumParticlesPerSpan = 5;
	bool bWorldCollision = true;
	bool bInitialized = false;
};
