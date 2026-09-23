#pragma once

#include "Components/SceneComponent.h"
#include "CollisionQueryParams.h"
#include "ChopItRopeComponent.generated.h"

class UChopItChainDefinition;
class UPrimitiveComponent;

/** A sliding contact, not a pinned wrap anchor. */
struct FChopItRopeContact
{
	TWeakObjectPtr<UPrimitiveComponent> Component;
	int32 Segment = 0;
	FVector Position = FVector::ZeroVector;
	FVector Normal = FVector::UpVector;
};

struct FChopItRopeBodyLoad
{
	TWeakObjectPtr<UPrimitiveComponent> Body;
	FVector Force = FVector::ZeroVector;
	FVector Torque = FVector::ZeroVector;
};

/** Sole physical authority for the chain, reel and accepted endpoints. */
UCLASS(ClassGroup = (ChopIt), meta = (BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItRopeComponent final : public USceneComponent
{
	GENERATED_BODY()
public:
	UChopItRopeComponent();
	void Configure(const UChopItChainDefinition* Definition);
	void InitializeRope(const FVector& StartWorld, const FVector& EndWorld, float InRopeLength, AActor* InIgnoredEndActor);
	void SetEndpoints(const FVector& StartWorld, const FVector& EndWorld);
	void SetRopeLength(float InRopeLength);
	void SetAutomaticReel(bool bEnabled) { bAutomaticReel = bEnabled; }
	void Simulate(float DeltaSeconds);
	void ResetRope();
	void ApplyForcesToPhysicsProps(float DeltaSeconds);
	TArray<FChopItRopeBodyLoad> CalculateBodyLoads() const;
	FString DescribeState() const;

	const TArray<FVector>& GetParticleLocations() const { return Positions; }
	const TArray<float>& GetSegmentRestLengths() const { return RestLengths; }
	const TArray<FChopItRopeContact>& GetContacts() const { return Contacts; }
	float GetRopeLength() const { return DeployedLength; }
	float GetStoredLength() const { return FMath::Max(0.0f, MaxLength - DeployedLength); }
	float GetSimulatedPathLength() const;
	float GetMaximumLengthError() const;
	float GetCollisionRadius() const { return Radius; }
	float GetEndpointTension() const { return EndpointTension; }
	float GetLastSimulationMilliseconds() const { return LastSimulationMilliseconds; }
	FVector GetAcceptedEndpoint() const { return Positions.IsEmpty() ? EndTarget : Positions.Last(); }
	FVector GetOutwardDirection() const;
	bool IsMovementBlocked() const { return bMovementBlocked; }
	bool IsFrameBudgetLimited() const { return bFrameBudgetLimited; }
	bool IsInitialized() const { return bInitialized; }
	bool IsCollisionFree(float AllowedPenetration = 0.0f) const;
	int32 GetRejectedStepCount() const { return RejectedSteps; }

private:
	bool IsFrameBudgetExhausted() const;
	double SimulationDeadline = 0.0;
	int32 BudgetLimitedFrames = 0;
	int32 AcceptedStepsThisFrame = 0;
	int32 IterationsThisFrame = 0;
	mutable int64 SweepQueriesThisFrame = 0;
	int64 ContactChecksThisFrame = 0;
	bool AdvanceStep(float Dt, const FVector& Start, const FVector& End, float Length);
	void SolveLengths(float Dt);
	void SolveTotalLength();
	FVector MoveParticle(int32 Index, const FVector& Target);
	bool ResizeAtOutlet(float Length);
	float GetContactGuidedLength(const FVector& Start, const FVector& End) const;
	void RefreshMasses();
	void RecordContact(int32 Segment, const FHitResult& Hit);
	bool Sweep(const FVector& A, const FVector& B, float QueryRadius, FHitResult& Hit) const;
	bool SegmentClear(const FVector& A, const FVector& B, float QueryRadius) const;
	bool TransitionClear(int32 Index, const FVector& Candidate) const;
	void RefreshContacts();
	bool ResolveInitialPenetrations();
	void RefreshCollisionBounds();
	bool MayHitWorld(const FVector& A, const FVector& B, float QueryRadius) const;

	TArray<FVector> Positions;
	TArray<FVector> PreviousPositions;
	TArray<float> RestLengths;
	TArray<float> InverseMasses;
	TArray<float> Multipliers;
	TArray<FVector> Directions;
	TArray<float> Diagonal, Upper, Rhs, DeltaLambda;
	TArray<FChopItRopeContact> Contacts;
	TArray<int32> ContactHeads, ContactNext;
	TArray<FBox> CollisionBounds;
	FBox CollisionQueryRegion = FBox(ForceInit);
	TArray<TArray<FPlane>> CollisionPlanes;
	bool bCollisionBoundsReady = false;
	FCollisionQueryParams QueryParams;
	FCollisionResponseParams ResponseParams;
	FVector StartTarget = FVector::ZeroVector;
	FVector EndTarget = FVector::ZeroVector;
	float DeployedLength = 0.0f, RequestedLength = 0.0f, MaxLength = 3600.0f;
	float Spacing = 12.0f, Radius = 6.25f, Skin = 0.5f;
	float MassPerCm = 0.02f, Compliance = 0.0000001f;
	float GravityScale = 1.0f, Damping = 0.02f, ConstraintDamping = 0.9f;
	float GroundFriction = 0.0f, ObstacleFriction = 0.08f;
	float StepTime = 1.0f / 120.0f, AccumulatedTime = 0.0f;
	float StretchTolerance = 10.0f, MinimumLength = 100.0f;
	float FeedSpeed = 1000.0f, FeedAcceleration = 3200.0f, ReelVelocity = 0.0f;
	float TakeUpScale = 1.0f;
	float Slack = 200.0f, Hysteresis = 20.0f;
	float MaximumForce = 250000.0f, ForceScale = 1.0f;
	float EndpointTension = 0.0f, LastSimulationMilliseconds = 0.0f;
	float LastConstraintStep = 1.0f / 120.0f;
	int32 Iterations = 32, MaximumSteps = 8, RejectedSteps = 0;
	int32 ResizeRejects = 0, EndpointRejects = 0, ConstraintRejects = 0;
	bool bCollision = true, bInitialized = false, bAutomaticReel = false, bMovementBlocked = false;
	bool bFrameBudgetLimited = false;
};
