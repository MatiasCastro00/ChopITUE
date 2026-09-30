#include "Economy/ChopItRopeComponent.h"

#include "ChopItCollision.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "Chaos/Convex.h"
#include "Economy/ChopItChainDefinition.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "HAL/PlatformTime.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

static TAutoConsoleVariable<float> CVarChainFrameBudgetMs(
	TEXT("ChopIt.Chain.FrameBudgetMs"), 32.0f,
	TEXT("CPU budget per chain per frame in milliseconds. 0 disables the budget for offline validation."));

static TAutoConsoleVariable<float> CVarChainDeathFrameBudgetMs(
	TEXT("ChopIt.Chain.DeathFrameBudgetMs"), 4.0f,
	TEXT("Hard CPU budget for the death reel solver per frame in milliseconds."));

static TAutoConsoleVariable<int32> CVarChainDeathMaxSubsteps(
	TEXT("ChopIt.Chain.DeathMaxSubsteps"), 6,
	TEXT("Maximum fixed rope substeps processed by the death reel in one frame."));

static TAutoConsoleVariable<int32> CVarChainDeathSolverIterations(
	TEXT("ChopIt.Chain.DeathSolverIterations"), 12,
	TEXT("Maximum constraint iterations per accepted death-reel substep."));

bool UChopItRopeComponent::IsFrameBudgetExhausted() const
{
	return SimulationDeadline > 0.0 && FPlatformTime::Seconds() >= SimulationDeadline;
}

UChopItRopeComponent::UChopItRopeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItRopeComponent::Configure(const UChopItChainDefinition* D)
{
	if (!D) return;
	MaxLength = FMath::Max(20.0f, D->MaxChainLength);
	MinimumLength = FMath::Clamp(D->MinimumDeployedLength, 20.0f, MaxLength);
	Radius = FMath::Max(1.0f, D->CableParticleDiameter * 0.5f);
	Skin = FMath::Clamp(D->CableCollisionSkin, 0.1f, 2.0f);
	Spacing = FMath::Clamp(D->PhysicalSegmentLength, 4.0f, FMath::Max(4.0f, Radius * 2.0f));
	MassPerCm = FMath::Max(0.0001f, D->LinearMassDensity / 100.0f);
	Compliance = FMath::Clamp(D->StretchCompliance, 0.0f, 0.00001f);
	GravityScale = FMath::Max(0.0f, D->CableGravityScale);
	Damping = FMath::Clamp(D->CableVelocityDamping, 0.0f, 0.25f);
	ConstraintDamping = FMath::Clamp(D->CableConstraintVelocityDamping, 0.0f, 1.0f);
	GroundFriction = FMath::Clamp(D->CableGroundFriction, 0.0f, 1.0f);
	ObstacleFriction = FMath::Clamp(D->CableCollisionFriction, 0.0f, 1.0f);
	Iterations = FMath::Clamp(D->CableSolverIterations, 8, 64);
	MaximumSteps = FMath::Clamp(D->CableMaximumSubsteps, 1, 16);
	// One clock for contacts, mass, reel and XPBD. Rendering never changes dt.
	StepTime = 1.0f / 120.0f;
	StretchTolerance = FMath::Clamp(D->ChainStretchTolerance, 0.1f, 50.0f);
	FeedSpeed = FMath::Max(20.0f, D->ChainFeedSpeed);
	FeedAcceleration = FMath::Max(20.0f, D->ChainFeedAcceleration);
	DeathRetractionSpeed = FMath::Max(0.0f, D->DeathRetractionMinSpeed);
	DeathFeedSpeedMultiplier = FMath::Clamp(D->DeathReelSpeedMultiplier, 1.0f, 10.0f);
	DeathFeedAccelerationMultiplier = FMath::Clamp(D->DeathReelAccelerationMultiplier, 1.0f, 10.0f);
	Slack = FMath::Max(0.0f, D->ChainSlack);
	Hysteresis = FMath::Max(0.0f, D->ChainReelHysteresis);
	MaximumForce = FMath::Max(0.0f, D->MaximumPropTensionForce);
	ForceScale = FMath::Max(0.0f, D->PhysicsPropForceScale);
	bCollision = D->bCableWorldCollision;
}

void UChopItRopeComponent::InitializeRope(const FVector& Start, const FVector& End, float Length, AActor* IgnoredActor)
{
	ResetRope();
	StartTarget = Start;
	EndTarget = End;
	QueryParams = FCollisionQueryParams(SCENE_QUERY_STAT(ChopItChainV2), false, GetOwner());
	QueryParams.bFindInitialOverlaps = true;
	if (IgnoredActor) QueryParams.AddIgnoredActor(IgnoredActor);
	ResponseParams = FCollisionResponseParams::DefaultResponseParam;
	for (ECollisionChannel Channel : {ECC_Pawn, ChopItCollisionChannels::Enemy,
		ChopItCollisionChannels::Projectile, ChopItCollisionChannels::Pickup,
		ChopItCollisionChannels::DeliveryZone, ChopItCollisionChannels::Chain})
	{
		ResponseParams.CollisionResponse.SetResponse(Channel, ECR_Ignore);
	}
	DeployedLength = FMath::Clamp(Length, MinimumLength, MaxLength);
	RequestedLength = DeployedLength;
	const int32 Count = FMath::Max(2, FMath::CeilToInt(DeployedLength / Spacing));
	for (int32 I = 0; I <= Count; ++I)
	{
		Positions.Add(FMath::Lerp(Start, End, static_cast<double>(I) / Count));
		if (I < Count) RestLengths.Add(DeployedLength / Count);
	}
	PreviousPositions = Positions;
	RefreshMasses();
	// Never create an already threaded-through-wall rope. The owner can retry
	// after the player returns to a valid deployment position.
	bInitialized = FVector::Distance(Start, End) <= DeployedLength + StretchTolerance && IsCollisionFree();
	if (!bInitialized)
	{
		Positions.Reset();
		PreviousPositions.Reset();
		RestLengths.Reset();
	}
}

void UChopItRopeComponent::SetEndpoints(const FVector& Start, const FVector& End)
{
	if (!Start.ContainsNaN() && !End.ContainsNaN())
	{
		StartTarget = Start;
		EndTarget = End;
	}
}

void UChopItRopeComponent::SetRopeLength(float Length)
{
	if (FMath::IsFinite(Length)) RequestedLength = FMath::Clamp(Length, MinimumLength, MaxLength);
}

void UChopItRopeComponent::SetDeathAllowedLength(float Length)
{
	if (!bCinematicRetraction || !bInitialized || !FMath::IsFinite(Length)) return;
	const float NewLength = FMath::Clamp(Length, MinimumLength, MaxLength);
	if (FMath::IsNearlyEqual(NewLength, DeployedLength, 0.0001f))
	{
		RequestedLength = NewLength;
		return;
	}
	const float Scale = NewLength / FMath::Max(DeployedLength, UE_SMALL_NUMBER);
	for (float& RestLength : RestLengths) RestLength *= Scale;
	DeployedLength = RequestedLength = NewLength;
	RefreshMasses();
}

bool UChopItRopeComponent::Sweep(const FVector& A, const FVector& B, float QueryRadius, FHitResult& Hit) const
{
	if (!bCollision || !GetWorld() || !MayHitWorld(A, B, QueryRadius)) return false;
	++SweepQueriesThisFrame;
	return GetWorld()->SweepSingleByChannel(Hit, A, B, FQuat::Identity,
		ChopItCollisionChannels::Chain, FCollisionShape::MakeSphere(QueryRadius), QueryParams, ResponseParams);
}

bool UChopItRopeComponent::MayHitWorld(const FVector& A, const FVector& B, float QueryRadius) const
{
	if (!bCollisionBoundsReady) return true;
	FBox QueryBounds(ForceInit);
	QueryBounds += A; QueryBounds += B;
	QueryBounds = QueryBounds.ExpandBy(QueryRadius);
	// A difficult solve may leave the cached broad-phase region. In that case
	// use the full world query instead of assuming uncollected bodies are absent.
	if (!CollisionQueryRegion.IsInside(QueryBounds)) return true;
	for (int32 I = 0; I < CollisionBounds.Num(); ++I)
	{
		if (!QueryBounds.Intersect(CollisionBounds[I])) continue;
		bool bSeparated = false;
		for (const FPlane& Plane : CollisionPlanes[I])
			if (Plane.PlaneDot(A) > QueryRadius && Plane.PlaneDot(B) > QueryRadius) { bSeparated = true; break; }
		if (!bSeparated) return true;
	}
	return false;
}

void UChopItRopeComponent::RefreshCollisionBounds()
{
	CollisionBounds.Reset();
	CollisionPlanes.Reset();
	bCollisionBoundsReady = false;
	if (!bCollision || !GetWorld() || Positions.IsEmpty()) return;
	FBox QueryBounds(ForceInit);
	for (const FVector& P : Positions) QueryBounds += P;
	QueryBounds += StartTarget; QueryBounds += EndTarget;
	// Enclose every possible correction in this bounded solve, not just the
	// current rope. This broad phase cannot hide a newly approached obstacle.
	QueryBounds = QueryBounds.ExpandBy(Radius * (Iterations + 4) + Skin + FeedSpeed * StepTime);
	CollisionQueryRegion = QueryBounds;
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByChannel(Overlaps, QueryBounds.GetCenter(), FQuat::Identity,
		ChopItCollisionChannels::Chain, FCollisionShape::MakeBox(QueryBounds.GetExtent()), QueryParams, ResponseParams);
	for (const FOverlapResult& O : Overlaps)
	{
		UPrimitiveComponent* Body = O.GetComponent();
		if (!O.bBlockingHit || !Body) continue;
		CollisionBounds.Add(Body->Bounds.GetBox());
		TArray<FPlane>& Planes = CollisionPlanes.AddDefaulted_GetRef();
		UBodySetup* Setup = Body->GetBodySetup();
		// Instanced, compound, complex and transformed elements keep the AABB
		// fallback; a component transform alone cannot describe their bodies.
		if (Body->GetClass() != UStaticMeshComponent::StaticClass() || Body->Mobility != EComponentMobility::Static || !Setup
			|| Setup->GetCollisionTraceFlag() == CTF_UseComplexAsSimple
			|| Setup->AggGeom.GetElementCount() != 1 || Setup->AggGeom.ConvexElems.Num() != 1
			|| Body->GetComponentScale().GetMin() < 0.01f) continue;
		const FKConvexElem& Element = Setup->AggGeom.ConvexElems[0];
		if (!Element.GetTransform().Equals(FTransform::Identity)) continue;
		const auto& Hull = Element.GetChaosConvexMesh();
		if (!Hull || Hull->NumVertices() == 0) continue;
		const FMatrix Transform = Element.GetTransform().ToMatrixWithScale() * Body->GetComponentTransform().ToMatrixWithScale();
		TArray<FVector, TInlineAllocator<128>> Vertices;
		for (int32 V = 0; V < Hull->NumVertices(); ++V) Vertices.Add(Transform.TransformPosition(FVector(Hull->GetVertex(V))));
		for (int32 P = 0; P < Hull->NumPlanes(); ++P)
		{
			Chaos::FVec3 Normal, Point;
			Hull->GetPlaneNX(P, Normal, Point);
			FPlane Plane = FPlane(FVector(Point), FVector(Normal)).TransformBy(Transform);
			Plane.Normalize();
			double Support = -TNumericLimits<double>::Max();
			for (const FVector& Vertex : Vertices) Support = FMath::Max(Support, FVector::DotProduct(Vertex, FVector(Plane)));
			Plane.W = Support + 0.01; // enclose the cooked hull, including roundoff
			Planes.Add(Plane);
		}
	}
	bCollisionBoundsReady = true;
}

bool UChopItRopeComponent::SegmentClear(const FVector& A, const FVector& B, float QueryRadius) const
{
	FHitResult Hit;
	return !Sweep(A, B, QueryRadius, Hit);
}

bool UChopItRopeComponent::IsCollisionFree(float AllowedPenetration) const
{
	if (!bCollision || !GetWorld()) return true;
	for (int32 I = 1; I < Positions.Num(); ++I)
	{
		FHitResult Hit;
		// Independent final validation deliberately bypasses our cached broad phase.
		if (GetWorld()->SweepSingleByChannel(Hit, Positions[I - 1], Positions[I], FQuat::Identity,
			ChopItCollisionChannels::Chain, FCollisionShape::MakeSphere(FMath::Max(0.1f, Radius - AllowedPenetration)),
			QueryParams, ResponseParams)) return false;
	}
	return true;
}

void UChopItRopeComponent::RecordContact(int32 Segment, const FHitResult& Hit)
{
	if (!Hit.GetComponent()) return;
	if (ContactHeads.Num() != RestLengths.Num() || ContactNext.Num() != Contacts.Num())
	{
		ContactHeads.Init(INDEX_NONE, RestLengths.Num());
		ContactNext.Init(INDEX_NONE, Contacts.Num());
		for (int32 I = 0; I < Contacts.Num(); ++I)
			if (ContactHeads.IsValidIndex(Contacts[I].Segment))
			{
				ContactNext[I] = ContactHeads[Contacts[I].Segment];
				ContactHeads[Contacts[I].Segment] = I;
			}
	}
	if (!ContactHeads.IsValidIndex(Segment)) return;
	// Deduplicate only this edge's contacts, not every contact on the chain.
	for (int32 I = ContactHeads[Segment]; I != INDEX_NONE; I = ContactNext[I])
	{
		FChopItRopeContact& C = Contacts[I];
		++ContactChecksThisFrame;
		if (C.Segment == Segment && C.Component == Hit.GetComponent()
			&& FVector::DotProduct(C.Normal, Hit.Normal) > 0.95)
		{
			C.Position = Hit.ImpactPoint;
			return;
		}
	}
	ContactNext.Add(ContactHeads[Segment]);
	ContactHeads[Segment] = Contacts.Num();
	FChopItRopeContact& Contact = Contacts.AddDefaulted_GetRef();
	Contact.Component = Hit.GetComponent();
	Contact.Segment = Segment;
	Contact.Position = Hit.ImpactPoint;
	Contact.Normal = Hit.Normal.GetSafeNormal(UE_SMALL_NUMBER, Hit.ImpactNormal);
}

bool UChopItRopeComponent::TransitionClear(int32 Index, const FVector& Candidate) const
{
	if (!bCollision || !GetWorld()) return true;
	for (int32 Neighbour : {Index - 1, Index + 1})
	{
		if (!Positions.IsValidIndex(Neighbour)) continue;
		const FVector Fixed = Positions[Neighbour];
		// A moving edge sweeps the triangle (fixed, old, candidate). Its tight
		// bounds are sufficient; inflating the old edge by total motion falsely
		// includes the floor for every horizontal correction of a resting chain.
		FBox SweptBounds(ForceInit);
		SweptBounds += Fixed; SweptBounds += Positions[Index]; SweptBounds += Candidate;
		SweptBounds = SweptBounds.ExpandBy(Radius + Skin * 0.25f);
		bool bMayHit = !bCollisionBoundsReady || !CollisionQueryRegion.IsInside(SweptBounds);
		for (int32 Obstacle = 0; Obstacle < CollisionBounds.Num(); ++Obstacle)
		{
			if (!SweptBounds.Intersect(CollisionBounds[Obstacle])) continue;
			bool bSeparated = false;
			for (const FPlane& Plane : CollisionPlanes[Obstacle])
				if (Plane.PlaneDot(Fixed) > Radius + Skin * .25f
					&& Plane.PlaneDot(Positions[Index]) > Radius + Skin * .25f
					&& Plane.PlaneDot(Candidate) > Radius + Skin * .25f) { bSeparated = true; break; }
			if (!bSeparated) { bMayHit = true; break; }
		}
		if (!bMayHit) continue;
		if (!SegmentClear(Fixed, Candidate, Radius + Skin * 0.25f)) return false;
		// The verified final capsule also contains the entire moving edge when
		// endpoint travel fits inside its skin. No extra temporal samples are
		// necessary for these small solver corrections, even at a contact.
		if (FVector::DistSquared(Positions[Index], Candidate) <= FMath::Square(Skin * 0.25f)) continue;
		// Cheap conservative sweep of the complete rotating edge. Only contacts
		// need the more expensive material-sample narrow phase below.
		const FVector OldEdge = Positions[Index] - Fixed;
		const float Motion = FVector::Distance(Positions[Index], Candidate);
		const float Cover = Radius + Motion * 0.5f;
		FHitResult CapsuleHit;
		if (!GetWorld()->SweepSingleByChannel(CapsuleHit, (Fixed + Positions[Index]) * 0.5,
			(Fixed + Candidate) * 0.5, FQuat::FindBetweenNormals(FVector::UpVector, OldEdge.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector)),
			ChopItCollisionChannels::Chain, FCollisionShape::MakeCapsule(Cover, OldEdge.Size() * 0.5f + Cover), QueryParams, ResponseParams)) continue;

		// Cover the entire swept edge, including its interior, by overlapping
		// swept spheres. The inflation covers the gaps between material samples.
		const float Length = FMath::Max(FVector::Distance(Fixed, Positions[Index]), FVector::Distance(Fixed, Candidate));
		// Keep the conservative cover inside the static contact skin. A larger
		// inflation introduces an invisible barrier at corners: the end capsule
		// is clear but every sliding transition is incorrectly rejected.
		const float MaxSampleSpacing = 2.0f * FMath::Sqrt(FMath::Square(Radius + Skin * 0.125f) - Radius * Radius);
		const int32 Samples = FMath::Max(1, FMath::CeilToInt(Length / MaxSampleSpacing));
		const float SampleSpacing = Length / Samples;
		const float CoverRadius = FMath::Sqrt(Radius * Radius + SampleSpacing * SampleSpacing * 0.25f);
		for (int32 S = 1; S <= Samples; ++S)
		{
			const double T = static_cast<double>(S) / Samples;
			const FVector A = FMath::Lerp(Fixed, Positions[Index], T);
			const FVector B = FMath::Lerp(Fixed, Candidate, T);
			FHitResult Hit;
			if (Sweep(A, B, CoverRadius, Hit))
			{
				// A shallow overlap of the conservative cover is not actual
				// penetration. Short moves + exact final capsule check permit sliding.
				if (Hit.bStartPenetrating && Hit.PenetrationDepth <= CoverRadius - Radius + Skin) continue;
				if (!Hit.bStartPenetrating && FVector::DotProduct(B - A, Hit.Normal) >= -0.0001) continue;
				return false;
			}
		}
	}
	return true;
}

FVector UChopItRopeComponent::MoveParticle(int32 Index, const FVector& Target)
{
	const FVector Old = Positions[Index];
	if (IsFrameBudgetExhausted()) return Old;
	if (Old.Equals(Target, 0.00001f)) return Old;
	const bool bFreeEndpoint = (Index == 0 || Index == Positions.Num() - 1) && DeployedLength < MaxLength - 0.1f;
	const float MaxMove = bFreeEndpoint ? Radius : Radius * 0.75f;
	FVector Candidate = Old + (Target - Old).GetClampedToMaxSize(MaxMove);
	if (!bCollision || !GetWorld()) return Candidate;
	FHitResult Hit;
	if (Sweep(Old, Candidate, Radius + Skin, Hit))
	{
		RecordContact(FMath::Clamp(Index - 1, 0, RestLengths.Num() - 1), Hit);
		const FVector Normal = Hit.Normal.GetSafeNormal(UE_SMALL_NUMBER, Hit.ImpactNormal);
		if (Hit.bStartPenetrating)
			Candidate += Normal * (Hit.PenetrationDepth + Skin * 0.1f);
		else
			Candidate = Hit.Location + Normal * Skin * 0.1f
				+ FVector::VectorPlaneProject(Candidate - Hit.Location, Normal);
	}
	// A particle may be clear while the edge to its neighbour cuts a corner.
	// Project the proposed endpoint onto that contact plane before line search.
	for (int32 Neighbour : {Index - 1, Index + 1})
	{
		if (!Positions.IsValidIndex(Neighbour)) continue;
		if (Sweep(Positions[Neighbour], Candidate, Radius + Skin * 0.5f, Hit))
		{
			RecordContact(FMath::Min(Index, Neighbour), Hit);
			const FVector N = Hit.Normal.GetSafeNormal(UE_SMALL_NUMBER, Hit.ImpactNormal);
			const float Separation = FVector::DotProduct(Hit.Location - Candidate, N);
			Candidate += N * FMath::Max(0.0f, Separation + Skin * 0.1f);
		}
	}
	Candidate = Old + (Candidate - Old).GetClampedToMaxSize(MaxMove);
	if (TransitionClear(Index, Candidate)) return Candidate;
	// Never replace a blocked edge by a newly computed shortest path.
	for (int32 Attempt = 0; Attempt < 7; ++Attempt)
	{
		if (IsFrameBudgetExhausted()) return Old;
		Candidate = (Candidate + Old) * 0.5;
		if (TransitionClear(Index, Candidate)) return Candidate;
	}
	return Old;
}

void UChopItRopeComponent::RefreshMasses()
{
	// Topology changes can make an old interior index the attached endpoint.
	// SetNumZeroed only clears new entries; reset existing endpoint masses too.
	InverseMasses.Init(0.0f, Positions.Num());
	for (int32 I = 1; I < Positions.Num() - 1; ++I)
		InverseMasses[I] = 1.0f / FMath::Max(0.0001f, MassPerCm * (RestLengths[I - 1] + RestLengths[I]) * 0.5f);
	Multipliers.SetNumZeroed(RestLengths.Num());
}

bool UChopItRopeComponent::ResizeAtOutlet(float Length)
{
	Length = FMath::Clamp(Length, MinimumLength, MaxLength);
	float Change = Length - DeployedLength;
	if (FMath::Abs(Change) < 0.0001f) return true;
	if (Change > 0)
	{
		RestLengths[0] += Change;
		while (RestLengths[0] > Spacing * 1.5f)
		{
			const float OldRest = RestLengths[0];
			const float Fraction = FMath::Min(0.5f, Spacing / OldRest);
			const FVector NewPosition = FMath::Lerp(Positions[0], Positions[1], Fraction);
			const FVector NewPrevious = FMath::Lerp(PreviousPositions[0], PreviousPositions[1], Fraction);
			Positions.Insert(NewPosition, 1);
			PreviousPositions.Insert(NewPrevious, 1);
			RestLengths[0] = Spacing;
			RestLengths.Insert(OldRest - Spacing, 1);
		}
	}
	else
	{
		float Remove = -Change;
		while (Remove >= RestLengths[0] && RestLengths.Num() > 2)
		{
			// Absorb a joint only when its entire replacement edge is clear and
			// the joint has physically reached the outlet.
			if (FVector::Distance(Positions[0], Positions[1]) > Spacing * 1.1f
				|| !SegmentClear(Positions[0], Positions[2], Radius + Skin * 0.25f)) return false;
			Remove -= RestLengths[0];
			RestLengths.RemoveAt(0);
			Positions.RemoveAt(1);
			PreviousPositions.RemoveAt(1);
		}
		RestLengths[0] -= Remove;
		if (RestLengths[0] < 0.1f && RestLengths.Num() > 2)
		{
			// Merge a vanishing outlet remainder without discarding its material
			// length. Rejecting the same sub-mm remainder each frame jams a reel.
			if (!SegmentClear(Positions[0], Positions[2], Radius + Skin * 0.25f)) return false;
			RestLengths[0] += RestLengths[1];
			RestLengths.RemoveAt(1);
			Positions.RemoveAt(1);
			PreviousPositions.RemoveAt(1);
		}
	}
	DeployedLength = Length;
	RefreshMasses();
	return true;
}

void UChopItRopeComponent::SolveLengths(float Dt)
{
	const int32 N = RestLengths.Num();
	Directions.SetNumUninitialized(N);
	Diagonal.SetNumUninitialized(N);
	Upper.SetNumUninitialized(N);
	Rhs.SetNumUninitialized(N);
	DeltaLambda.SetNumUninitialized(N);
	const float Alpha = Compliance / (Dt * Dt);
	TArray<bool, TInlineAllocator<512>> Active;
	Active.SetNumZeroed(N);
	TArray<FVector, TInlineAllocator<512>> ContactNormals;
	ContactNormals.Init(FVector::ZeroVector, Positions.Num());
	for (int32 I = 0; I < N; ++I)
	{
		const FVector Edge = Positions[I + 1] - Positions[I];
		Directions[I] = Edge.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		Active[I] = Edge.Size() > RestLengths[I] || Multipliers[I] < 0;
	}
	const auto Project = [&](int32 Joint, const FVector& V)
	{
		return V - ContactNormals[Joint] * FVector::DotProduct(V, ContactNormals[Joint]);
	};
	// First solve freely to identify loaded surfaces, then solve J W J^T with
	// their normal degrees of freedom removed. Projecting only the resulting
	// positions wastes iterations pushing into a floor and can jam long chains.
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (int32 I = 0; I < N; ++I)
		{
			Diagonal[I] = Active[I] ? InverseMasses[I] * FVector::DotProduct(Directions[I], Project(I, Directions[I]))
				+ InverseMasses[I + 1] * FVector::DotProduct(Directions[I], Project(I + 1, Directions[I])) + Alpha + 0.00001f : 1.0f;
			Rhs[I] = Active[I] ? -(FVector::Distance(Positions[I], Positions[I + 1]) - RestLengths[I]) - Alpha * Multipliers[I] : 0;
			if (I > 0)
			{
				const float Lower = Active[I] && Active[I - 1] ? -InverseMasses[I] * FVector::DotProduct(Directions[I - 1], Project(I, Directions[I])) : 0;
				Upper[I - 1] = Lower;
				const float Factor = Lower / FMath::Max(0.00001f, Diagonal[I - 1]);
				Diagonal[I] -= Factor * Upper[I - 1];
				Rhs[I] -= Factor * Rhs[I - 1];
			}
		}
		for (int32 I = N - 1; I >= 0; --I)
		{
			DeltaLambda[I] = (Rhs[I] - (I + 1 < N ? Upper[I] * DeltaLambda[I + 1] : 0.0f)) / FMath::Max(0.00001f, Diagonal[I]);
			DeltaLambda[I] = FMath::Min(DeltaLambda[I], -Multipliers[I]);
		}
		if (Pass == 0)
		{
			for (const FChopItRopeContact& C : Contacts)
				for (int32 I : {C.Segment, C.Segment + 1})
				{
					if (I <= 0 || I >= N || FVector::DotProduct(Positions[I] - C.Position, C.Normal) > Radius + Skin * 2) continue;
					const FVector Force = Directions[I - 1] * DeltaLambda[I - 1] - Directions[I] * DeltaLambda[I];
					if (FVector::DotProduct(Force, C.Normal) < -0.000001) ContactNormals[I] = C.Normal;
				}
		}
	}
	float TrustScale = 1.0f;
	for (int32 I = 1; I < N; ++I)
	{
		const FVector Correction = InverseMasses[I] * Project(I, Directions[I - 1] * DeltaLambda[I - 1] - Directions[I] * DeltaLambda[I]);
		TrustScale = FMath::Min(TrustScale, static_cast<float>(Radius * 0.5f / FMath::Max(0.001, Correction.Size())));
	}

	for (float& DL : DeltaLambda) DL *= TrustScale;
	for (int32 I = 1; I < N; ++I)
	{
		const FVector Correction = InverseMasses[I] * Project(I, Directions[I - 1] * DeltaLambda[I - 1] - Directions[I] * DeltaLambda[I]);
		const FVector Old = Positions[I];
		Positions[I] = MoveParticle(I, Old + Correction);
		PreviousPositions[I] += (Positions[I] - Old) * ConstraintDamping;
	}
	for (int32 I = 0; I < N; ++I) Multipliers[I] += DeltaLambda[I];
	// Locally relax only unconverged contact blocks. Running a scalar sweep on
	// already converged blocks adds directional bias when the reel reverses.
	if (GetMaximumLengthError() <= 0.5f) return;
	for (int32 K = 0; K < N; ++K)
	{
		const int32 I = ReelVelocity < 0 ? K : N - 1 - K;
		const FVector Edge = Positions[I + 1] - Positions[I];
		const float Length = Edge.Size();
		if (Length < UE_SMALL_NUMBER) continue;
		const FVector Direction = Edge / Length;
		const FVector LeftGradient = Project(I, -Direction), RightGradient = Project(I + 1, Direction);
		const float Weight = InverseMasses[I] * LeftGradient.SizeSquared() + InverseMasses[I + 1] * RightGradient.SizeSquared();
		if (Weight < UE_SMALL_NUMBER) continue;
		float DL = FMath::Min(-(Length - RestLengths[I] + Alpha * Multipliers[I]) / (Weight + Alpha), -Multipliers[I]);
		const float MaxMove = FMath::Max(InverseMasses[I] * LeftGradient.Size(), InverseMasses[I + 1] * RightGradient.Size()) * FMath::Abs(DL);
		DL *= FMath::Min(1.0f, Radius * 0.5f / FMath::Max(0.001f, MaxMove));
		for (int32 Joint : {I, I + 1})
		{
			if (InverseMasses[Joint] <= 0) continue;
			const FVector Old = Positions[Joint];
			Positions[Joint] = MoveParticle(Joint, Old + InverseMasses[Joint] * (Joint == I ? LeftGradient : RightGradient) * DL);
			PreviousPositions[Joint] += (Positions[Joint] - Old) * ConstraintDamping;
		}
		Multipliers[I] += DL;
	}
}

void UChopItRopeComponent::SolveTotalLength()
{
	// Local compliant constraints can each converge with a tiny extension,
	// whose sum exceeds the global material limit on a long folded chain.
	// Solve that hard limit as a constraint too, rather than rejecting a fully
	// converged local solve forever. Every correction uses swept collisions.
	const float TotalError = GetSimulatedPathLength() - DeployedLength;
	if (TotalError <= FMath::Min(StretchTolerance, 2.0f)) return;
	const float Excess = TotalError - FMath::Min(0.25f, StretchTolerance * 0.25f);
	// Resolve local bends before correcting the accumulated compliant residual.
	if (GetMaximumLengthError() > FMath::Min(0.2f, StretchTolerance * 0.25f)) return;
	TArray<FVector, TInlineAllocator<512>> Gradients;
	Gradients.Init(FVector::ZeroVector, Positions.Num());
	for (int32 I = 1; I < Positions.Num() - 1; ++I)
		Gradients[I] = (Positions[I] - Positions[I-1]).GetSafeNormal()
			- (Positions[I+1] - Positions[I]).GetSafeNormal();
	for (const FChopItRopeContact& Contact : Contacts)
		for (int32 I : {Contact.Segment, Contact.Segment + 1})
			if (I > 0 && I < Positions.Num() - 1 && FVector::DotProduct(-Gradients[I], Contact.Normal) < 0)
				Gradients[I] = FVector::VectorPlaneProject(Gradients[I], Contact.Normal);
	float Weight = 0, MaxWeightedGradient = 0;
	for (int32 I = 1; I < Positions.Num() - 1; ++I)
	{
		Weight += InverseMasses[I] * Gradients[I].SizeSquared();
		MaxWeightedGradient = FMath::Max(MaxWeightedGradient, InverseMasses[I] * static_cast<float>(Gradients[I].Size()));
	}
	if (Weight <= UE_SMALL_NUMBER) return;
	const float Lambda = FMath::Min(Excess / Weight, Radius * 0.5f / FMath::Max(MaxWeightedGradient, UE_SMALL_NUMBER));
	for (int32 I = 1; I < Positions.Num() - 1; ++I)
	{
		const FVector Old = Positions[I];
		Positions[I] = MoveParticle(I, Old - Gradients[I] * (InverseMasses[I] * Lambda));
		PreviousPositions[I] += (Positions[I] - Old) * ConstraintDamping;
	}
}

bool UChopItRopeComponent::AdvanceStep(float Dt, const FVector& Start, const FVector& End, float Length)
{
	if (IsFrameBudgetExhausted()) return false;
	RefreshCollisionBounds();
	const TArray<FVector> SavedPositions = Positions;
	const TArray<FVector> SavedPrevious = PreviousPositions;
	const TArray<float> SavedRest = RestLengths;
	const TArray<float> SavedLambda = Multipliers;
	const TArray<FChopItRopeContact> SavedContacts = Contacts;
	const float SavedLength = DeployedLength;
	const auto Reject = [&]()
	{
		Positions = SavedPositions;
		PreviousPositions = SavedPrevious;
		RestLengths = SavedRest;
		DeployedLength = SavedLength;
		RefreshMasses();
		Multipliers = SavedLambda;
		Contacts = SavedContacts;
		ContactHeads.Reset(); // rollback can restore different contacts of equal count
		++RejectedSteps;
		return false;
	};
	if (!ResizeAtOutlet(Length)) { ++ResizeRejects; return Reject(); }
	Contacts.Reset();
	Multipliers.Init(0.0f, RestLengths.Num());
	Positions[0] = MoveParticle(0, Start);
	Positions.Last() = MoveParticle(Positions.Num() - 1, End);
	if (!Positions[0].Equals(Start, 0.01f) || !Positions.Last().Equals(End, 0.01f)) { ++EndpointRejects; return Reject(); }
	PreviousPositions[0] = Positions[0];
	PreviousPositions.Last() = Positions.Last();
	const FVector Gravity(0, 0, (GetWorld() ? GetWorld()->GetGravityZ() : -980.0f) * GravityScale);
	for (int32 I = 1; I < Positions.Num() - 1; ++I)
	{
		const FVector Old = Positions[I];
		const FVector Velocity = (Old - PreviousPositions[I]) * (Dt / LastConstraintStep) * FMath::Pow(1.0f - Damping, Dt / StepTime);
		Positions[I] = MoveParticle(I, Old + Velocity + Gravity * Dt * Dt);
		PreviousPositions[I] = Old;
	}
	float PreviousLocalError = TNumericLimits<float>::Max();
	float PreviousTotalError = TNumericLimits<float>::Max();
	int32 StableIterations = 0;
	const int32 SolverIterations = bCinematicRetraction
		? FMath::Clamp(CVarChainDeathSolverIterations.GetValueOnGameThread(), 4, Iterations * 2)
		: Iterations * 2;
	for (int32 Iteration = 0; Iteration < SolverIterations; ++Iteration)
	{
		if (IsFrameBudgetExhausted()) break;
		++IterationsThisFrame;
		SolveLengths(Dt);
		SolveTotalLength();
		const float LocalError = GetMaximumLengthError();
		const float TotalError = GetSimulatedPathLength() - DeployedLength;
		if (LocalError < FMath::Min(0.2f, StretchTolerance * 0.25f)
			&& TotalError <= FMath::Min(0.5f, StretchTolerance * 0.5f)) break;
		// Compliant local constraints can converge to a small accumulated
		// residual. Stop only when it is already within the unchanged final
		// limits and repeated iterations no longer improve either error.
		const bool bStable = LocalError < FMath::Min(0.2f, StretchTolerance * 0.25f)
			&& TotalError <= FMath::Min(StretchTolerance, 2.0f)
			&& FMath::Abs(LocalError - PreviousLocalError) < 0.0001f
			&& FMath::Abs(TotalError - PreviousTotalError) < 0.001f;
		StableIterations = bStable ? StableIterations + 1 : 0;
		if (StableIterations >= 3) break;
		PreviousLocalError = LocalError;
		PreviousTotalError = TotalError;
	}
	// A valid step satisfies both constraints together, not just a final length
	// projection. A failed payout/retraction restores all topology and velocity.
	if (GetSimulatedPathLength() > DeployedLength + FMath::Min(StretchTolerance, 2.0f)
		|| GetMaximumLengthError() > FMath::Min(StretchTolerance, 1.0f)
		|| !IsCollisionFree()) {
		++ConstraintRejects;
		return Reject();
	}
	++AcceptedStepsThisFrame;
	LastConstraintStep = Dt;
	EndpointTension = Multipliers.IsEmpty() ? 0.0f : FMath::Max(0.0f, -Multipliers.Last() / (Dt * Dt));
	RefreshContacts();
	for (const FChopItRopeContact& C : Contacts)
	{
		for (int32 I : {C.Segment, C.Segment + 1})
		{
			if (I <= 0 || I >= Positions.Num() - 1) continue;
			FVector Velocity = Positions[I] - PreviousPositions[I];
			UPrimitiveComponent* Body = C.Component.Get();
			const FVector SurfaceVelocity = Body ? Body->GetPhysicsLinearVelocityAtPoint(C.Position) * Dt : FVector::ZeroVector;
			Velocity -= SurfaceVelocity;
			Velocity -= C.Normal * FMath::Min(0.0, FVector::DotProduct(Velocity, C.Normal));
			const float Friction = 1.0f - FMath::Pow(1.0f - (C.Normal.Z > 0.65 ? GroundFriction : ObstacleFriction), Dt / StepTime);
			Velocity -= FVector::VectorPlaneProject(Velocity, C.Normal) * Friction;
			PreviousPositions[I] = Positions[I] - Velocity - SurfaceVelocity;
		}
	}
	return true;
}

void UChopItRopeComponent::RefreshContacts()
{
	Contacts.Reset();
	for (int32 I = 0; I < RestLengths.Num(); ++I)
	{
		FHitResult Hit;
		if (Sweep(Positions[I], Positions[I + 1], Radius + Skin * 1.5f, Hit)) RecordContact(I, Hit);
	}
}

float UChopItRopeComponent::GetContactGuidedLength(const FVector& Start, const FVector& End) const
{
	// A folded last link is not a request for more material. Measure endpoint
	// travel relative to ordered contacts of the physical chain instead. Floor
	// support does not pin loose material; side/ceiling contacts retain wraps.
	TArray<bool, TInlineAllocator<512>> Supported;
	Supported.Init(false, Positions.Num());
	for (const FChopItRopeContact& Contact : Contacts)
	{
		if (!Contact.Component.IsValid() || Contact.Normal.Z > 0.65f) continue;
		for (int32 Joint : {Contact.Segment, Contact.Segment + 1})
			if (Joint > 0 && Joint < Positions.Num() - 1) Supported[Joint] = true;
	}
	FVector Last = Start;
	float RequiredLength = 0.0f;
	for (int32 Joint = 1; Joint < Positions.Num() - 1; ++Joint)
	{
		if (!Supported[Joint]) continue;
		RequiredLength += FVector::Distance(Last, Positions[Joint]);
		Last = Positions[Joint];
	}
	RequiredLength += FVector::Distance(Last, End);
	// Only changes in this length drive the motor. No contacts are reconstructed;
	// every material change still passes the complete physical solve.
	return RequiredLength;
}

void UChopItRopeComponent::Simulate(float DeltaSeconds)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ChopItChainV2);
	if (!bInitialized || DeltaSeconds <= 0 || !FMath::IsFinite(DeltaSeconds)) return;
	const double StartTime = FPlatformTime::Seconds();
	AcceptedStepsThisFrame = 0;
	IterationsThisFrame = 0;
	SweepQueriesThisFrame = ContactChecksThisFrame = 0;
	bFrameBudgetLimited = false;
	const float BudgetMs = bCinematicRetraction
		? CVarChainDeathFrameBudgetMs.GetValueOnGameThread()
		: CVarChainFrameBudgetMs.GetValueOnGameThread();
	SimulationDeadline = BudgetMs > 0.0f ? StartTime + BudgetMs * 0.001 : 0.0;
	ON_SCOPE_EXIT
	{
		if (bCinematicRetraction && Positions.Num() > 1)
		{
			// Preserve the player connection even if penetration recovery or the
			// frame budget exits before the constraint solve finishes.
			const FVector Shift = EndTarget - Positions.Last();
			Positions.Last() = EndTarget;
			PreviousPositions.Last() += Shift;
		}
		if (IsFrameBudgetExhausted())
		{
			bFrameBudgetLimited = true;
			++BudgetLimitedFrames;
			// Drop simulation debt: a slow frame must not cause a catch-up spiral.
			AccumulatedTime = 0.0f;
			// If nothing could advance, do not replay the same incompatible
			// inertial prediction forever. The motor can still pay out next frame.
			if (AcceptedStepsThisFrame == 0) PreviousPositions = Positions;
			// AdvanceStep already restored its last valid velocity and reel state.
			// Keep them: resetting the motor here makes repeated overload prevent
			// payout altogether, even when earlier substeps made valid progress.
			bMovementBlocked = true;
		}
		SimulationDeadline = 0.0;
		LastSimulationMilliseconds = static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0);
	};
	RefreshCollisionBounds();
	bMovementBlocked = !ResolveInitialPenetrations();
	if (bMovementBlocked) return;
	RefreshContacts();
	const int32 AllowedSteps = bCinematicRetraction
		? FMath::Clamp(CVarChainDeathMaxSubsteps.GetValueOnGameThread(), 1, MaximumSteps)
		: MaximumSteps;
	AccumulatedTime = FMath::Min(AccumulatedTime + DeltaSeconds, StepTime * AllowedSteps);
	const int32 Steps = FMath::Min(AllowedSteps, FMath::FloorToInt((AccumulatedTime + 0.000001f) / StepTime));
	const FVector FrameStart = Positions[0], FrameEnd = Positions.Last();
	const bool bStationaryEndpoint = FrameEnd.Equals(EndTarget, 0.1f);
	for (int32 S = 0; S < Steps; ++S)
	{
		if (IsFrameBudgetExhausted()) break;
		const FVector DesiredStart = FMath::Lerp(FrameStart, StartTarget, static_cast<double>(S + 1) / Steps);
		const FVector DesiredEnd = FMath::Lerp(FrameEnd, EndTarget, static_cast<double>(S + 1) / Steps);
		const FVector OldEnd = Positions.Last();
		const float OutwardTravel = FVector::DotProduct(DesiredEnd - OldEnd, GetOutwardDirection());
		if (bAutomaticReel)
		{
			const float GuidedLength = GetContactGuidedLength(DesiredStart, DesiredEnd);
			const float GuidedTravel = GuidedLength
				- GetContactGuidedLength(DesiredStart, OldEnd);
			const float DirectLength = static_cast<float>(FVector::Distance(DesiredStart, DesiredEnd));
			// Advance the motor demand, not the amount already deployed: loose
			// material left after a return must be reused on the next outward leg.
			RequestedLength = !bStationaryEndpoint
				? FMath::Clamp(FMath::Max(RequestedLength + GuidedTravel, GuidedLength + Slack), MinimumLength, MaxLength)
				: FMath::Clamp(DirectLength + Slack, MinimumLength, MaxLength);
			// Moving around a support can reduce direct endpoint distance while
			// existing wraps still consume material. Do not command the reel to
			// pull that material back through the obstacle.
			// Use the signed endpoint travel against the same contact set for
			// both directions. Losing a side contact to a floor query must not
			// abruptly subtract an entire wrap from the motor's demand.
		}
		const float Difference = RequestedLength - DeployedLength;
		// Hysteresis gates the motor, not its integrated destination. Resetting
		// the destination here discards every small inward movement forever.
		const float MotorDifference = bAutomaticReel && Difference < 0 && -Difference < Hysteresis ? 0 : Difference;
		const float ActiveFeedSpeed = bCinematicRetraction ? GetCinematicFeedSpeed() : FeedSpeed;
		const float ActiveFeedAcceleration = FeedAcceleration * (bCinematicRetraction ? DeathFeedAccelerationMultiplier : 1.0f);
		// Death is a commanded reel, not the normal endpoint-following motor.
		// Keep requesting full take-up until the controller's desired length is
		// actually accepted; a rejected solve leaves the difference as debt.
		const float WantedSpeed = bCinematicRetraction && MotorDifference < 0.0f
			? -ActiveFeedSpeed
			: FMath::Sign(MotorDifference)
				* FMath::Min(ActiveFeedSpeed, FMath::Sqrt(2.0f * ActiveFeedAcceleration * FMath::Abs(MotorDifference)));
		ReelVelocity = FMath::FInterpConstantTo(ReelVelocity, WantedSpeed, StepTime, ActiveFeedAcceleration);
		const float MotorScale = ReelVelocity < 0.0f && bStationaryEndpoint && !bCinematicRetraction ? TakeUpScale : 1.0f;
		float LengthChange = FMath::Clamp(ReelVelocity * StepTime * MotorScale, -FMath::Abs(Difference), FMath::Abs(Difference));
		if (LengthChange < 0.0f && !bStationaryEndpoint && !bCinematicRetraction)
		{
			// While the player moves, take up only a fraction of the inward
			// progress toward the fixed machine. Euclidean distance alone can
			// fall quickly as the endpoint turns around an obstacle, tightening
			// the remaining wrapped chain against the player. Any extra slack is
			// collected gradually after the endpoint settles.
			const float DirectInwardTravel = FMath::Max(0.0f,
				static_cast<float>(FVector::Distance(DesiredStart, OldEnd)
						- FVector::Distance(DesiredStart, DesiredEnd)));
			LengthChange = FMath::Max(LengthChange, -DirectInwardTravel * 0.5f);
			// This cap is the motor's actual take-up speed. Retaining the old
			// (much faster) requested speed invents braking time before payout
			// when the player turns around an obstacle, exhausting the slack.
			ReelVelocity = LengthChange / StepTime;
		}
		const float ProposedLength = FMath::Clamp(DeployedLength + LengthChange, MinimumLength, MaxLength);
		const float Travel = FMath::Max(FVector::Distance(Positions[0], DesiredStart), FVector::Distance(Positions.Last(), DesiredEnd));
		// Endpoint motion is already swept over the complete adjacent edge.
		// One radius fits its movement cap; normal walking therefore keeps the
		// configured 120 Hz clock instead of unnecessarily solving at 240 Hz.
		// At the hard material limit keep the finer solve: the endpoint must
		// slide along a taut constraint without a larger proposed step jamming it.
		const float MotionFraction = ProposedLength >= MaxLength - 0.1f ? 0.5f : 0.9f;
		const int32 MotionSteps = FMath::Clamp(FMath::CeilToInt(Travel / FMath::Max(1.0f, Radius * MotionFraction)), 1, 16);
		const FVector SubStart = Positions[0], SubEnd = Positions.Last();
		const float InitialLength = DeployedLength;
		for (int32 M = 0; M < MotionSteps; ++M)
		{
			if (IsFrameBudgetExhausted()) break;
			const double T = static_cast<double>(M + 1) / MotionSteps;
			const FVector A = FMath::Lerp(SubStart, DesiredStart, T);
			const FVector B = FMath::Lerp(SubEnd, DesiredEnd, T);
			const float L = FMath::Lerp(InitialLength, ProposedLength, T);
			const float Dt = StepTime / MotionSteps;
			const bool bTakingUp = L < DeployedLength;
			bool bAccepted = AdvanceStep(Dt, A, B, L);
			if (bTakingUp && bStationaryEndpoint && !bCinematicRetraction)
				TakeUpScale = bAccepted ? FMath::Min(1.0f, TakeUpScale * 1.05f) : FMath::Max(0.001f, TakeUpScale * 0.5f);
			if (IsFrameBudgetExhausted()) break;
			if (!bAccepted && L < DeployedLength)
			{
				if (!bStationaryEndpoint)
					for (float Fraction = 0.5f; !bAccepted && !IsFrameBudgetExhausted()
						&& Fraction >= (bCinematicRetraction ? 0.25f : 0.125f); Fraction *= 0.5f)
						bAccepted = AdvanceStep(Dt, A, B, FMath::Lerp(DeployedLength, L, Fraction));
				if (!bAccepted) bAccepted = AdvanceStep(Dt, A, B, DeployedLength);
			}
			if (!bAccepted)
			{
				// Payout is still rate-limited. Request enough for the next step,
				// rather than inventing instantaneous rope when a bend appears.
				if (bAutomaticReel && OutwardTravel > 0)
				{
					RequestedLength = FMath::Min(MaxLength, FMath::Max(RequestedLength, DeployedLength + Slack));

				}
				const FVector OldA = Positions[0], OldB = Positions.Last();
				for (float Fraction = 0.5f; !IsFrameBudgetExhausted()
					&& Fraction >= (bCinematicRetraction ? 0.125f : 1.0f / 64.0f); Fraction *= 0.5f)
				{
					if (AdvanceStep(Dt, FMath::Lerp(OldA, A, Fraction), FMath::Lerp(OldB, B, Fraction), FMath::Max(L, DeployedLength)))
					{
						bAccepted = true;
						break;
					}
				}
				bMovementBlocked = !Positions.Last().Equals(B, 0.1f);
				if (!bAccepted && L > DeployedLength) bAccepted = AdvanceStep(Dt, OldA, OldB, L);
				if (!bAccepted && !IsFrameBudgetExhausted())
				{
					// Restoring the same incompatible velocity would replay the failed
					// prediction forever. Brake the rejected material motion at the
					// last valid shape; the next step can slide or pay out normally.
					PreviousPositions = Positions;
					ReelVelocity = 0.0f;
				}
				break;
			}
		}
		AccumulatedTime = FMath::Max(0.0f, AccumulatedTime - StepTime);
	}
	bMovementBlocked |= !Positions.Last().Equals(EndTarget, 0.5f);
	LastSimulationMilliseconds = static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0);
}

TArray<FChopItRopeBodyLoad> UChopItRopeComponent::CalculateBodyLoads() const
{
	TArray<FChopItRopeBodyLoad> Result;
	TMap<UPrimitiveComponent*, TSet<int32>> BodyJoints;
	for (const FChopItRopeContact& C : Contacts)
	{
		UPrimitiveComponent* Body = C.Component.Get();
		if (!Body || !Body->IsSimulatingPhysics()) continue;
		for (int32 I : {C.Segment, C.Segment + 1})
			if (I > 0 && I < Positions.Num() - 1) BodyJoints.FindOrAdd(Body).Add(I);
	}
	for (const auto& Entry : BodyJoints)
	{
		FChopItRopeBodyLoad Load;
		Load.Body = Entry.Key;
		float TotalForceMagnitude = 0;
		for (int32 I : Entry.Value)
		{
			const float Left = FMath::Max(0.0f, -Multipliers[I - 1] / FMath::Square(LastConstraintStep));
			const float Right = FMath::Max(0.0f, -Multipliers[I] / FMath::Square(LastConstraintStep));
			const FVector Force = ((Positions[I - 1] - Positions[I]).GetSafeNormal() * Left
				+ (Positions[I + 1] - Positions[I]).GetSafeNormal() * Right) * ForceScale;
			Load.Force += Force;
			Load.Torque += FVector::CrossProduct(Positions[I] - Entry.Key->GetCenterOfMass(), Force);
			TotalForceMagnitude += Force.Size();
		}
		// Limit the aggregate, including cancelling forces which produce torque.
		const float Scale = FMath::Min(1.0f, MaximumForce / FMath::Max(1.0f, TotalForceMagnitude));
		Load.Force *= Scale;
		Load.Torque *= Scale;
		Result.Add(Load);
	}
	return Result;
}

void UChopItRopeComponent::ApplyForcesToPhysicsProps(float DeltaSeconds)
{
	if (DeltaSeconds <= 0 || MaximumForce <= 0 || ForceScale <= 0) return;
	for (const FChopItRopeBodyLoad& Load : CalculateBodyLoads())
	{
		if (!Load.Body.IsValid()) continue;
		// Chaos supplies inverse mass and inertia. Never cancel them by scaling
		// the force with mass, and never apply the same joint force twice.
		Load.Body->AddForce(Load.Force);
		Load.Body->AddTorqueInRadians(Load.Torque);
	}
}

bool UChopItRopeComponent::ResolveInitialPenetrations()
{
	if (IsCollisionFree()) return true;
	// A dynamic obstacle may have entered the chain during the last Chaos
	// step. Resolve its actual penetration before attempting player/reel work.
	for (int32 Pass = 0; Pass < 16; ++Pass)
	{
		if (IsFrameBudgetExhausted()) return false;
		for (int32 I = 0; I < RestLengths.Num(); ++I)
		{
			if (IsFrameBudgetExhausted()) return false;
			FHitResult Hit;
			if (!Sweep(Positions[I], Positions[I + 1], Radius + Skin * 0.25f, Hit)) continue;
			const FVector N = Hit.Normal.GetSafeNormal(UE_SMALL_NUMBER, Hit.ImpactNormal);
			const float Depth = Hit.bStartPenetrating ? Hit.PenetrationDepth + Skin
				: FMath::Max(0.0, FVector::DotProduct(Hit.Location - Positions[I + 1], N)) + Skin;
			const FVector Correction = N * Depth;
			// Escape the body that entered this edge, while still sweeping every
			// correction against the rest of the world. A moving prop must not
			// project the rope through a neighbouring wall.
			const FCollisionQueryParams SavedQuery = QueryParams;
			QueryParams.AddIgnoredComponent(Hit.GetComponent());
			for (int32 Joint : {I, I + 1})
			{
				if (Joint == 0) continue;
				const FVector Old = Positions[Joint];
				Positions[Joint] = MoveParticle(Joint, Old + Correction);
				PreviousPositions[Joint] += Positions[Joint] - Old;
			}
			QueryParams = SavedQuery;
		}
		if (IsCollisionFree())
		{
			RefreshContacts();
			return true;
		}
	}
	return false;
}

float UChopItRopeComponent::GetSimulatedPathLength() const
{
	float Length = 0;
	for (int32 I = 1; I < Positions.Num(); ++I) Length += FVector::Distance(Positions[I - 1], Positions[I]);
	return Length;
}

FVector UChopItRopeComponent::GetDeathConstrainedEndpoint() const
{
	if (!bCinematicRetraction || Positions.Num() < 2) return GetAcceptedEndpoint();
	float Excess = GetSimulatedPathLength() - DeployedLength;
	if (Excess <= 0.01f) return Positions.Last();
	for (int32 Index = Positions.Num() - 2; Index >= 0; --Index)
	{
		const FVector From = Positions[Index + 1];
		const FVector To = Positions[Index];
		const float SegmentLength = static_cast<float>(FVector::Distance(From, To));
		if (Excess <= SegmentLength)
			return FMath::Lerp(From, To, Excess / FMath::Max(SegmentLength, UE_SMALL_NUMBER));
		Excess -= SegmentLength;
	}
	return Positions[0];
}

void UChopItRopeComponent::AttachDeathEndpoint(const FVector& EndWorld)
{
	if (!bCinematicRetraction || Positions.Num() < 2 || EndWorld.ContainsNaN()) return;
	const FVector Shift = EndWorld - Positions.Last();
	EndTarget = EndWorld;
	Positions.Last() = EndWorld;
	PreviousPositions.Last() += Shift;
}

FString UChopItRopeComponent::DescribeState() const
{
	int32 ConvexBodies = 0;
	for (const TArray<FPlane>& Planes : CollisionPlanes) ConvexBodies += !Planes.IsEmpty();
	return FString::Printf(TEXT("ChainV2: deployed %.2f target %.2f motor %.2f, joints %d, outlet rest %.3f actual %.3f, rejected resize/end/constraints %d/%d/%d, CPU %.2f ms budget frames %d, sweeps %lld contact checks %lld, iterations %d steps %d local %.3f total %.3f convex %d"),
		DeployedLength, RequestedLength, ReelVelocity, Positions.Num(), RestLengths.IsEmpty() ? 0 : RestLengths[0],
		Positions.Num() < 2 ? 0 : FVector::Distance(Positions[0], Positions[1]), ResizeRejects, EndpointRejects, ConstraintRejects, LastSimulationMilliseconds, BudgetLimitedFrames,
		static_cast<long long>(SweepQueriesThisFrame), static_cast<long long>(ContactChecksThisFrame),
		IterationsThisFrame, AcceptedStepsThisFrame, GetMaximumLengthError(), GetSimulatedPathLength() - DeployedLength, ConvexBodies);
}

float UChopItRopeComponent::GetMaximumLengthError() const
{
	float Error = 0;
	for (int32 I = 0; I < RestLengths.Num(); ++I)
		Error = FMath::Max(Error, static_cast<float>(FVector::Distance(Positions[I], Positions[I + 1])) - RestLengths[I]);
	return Error;
}

FVector UChopItRopeComponent::GetOutwardDirection() const
{
	return Positions.Num() > 1 ? (Positions.Last() - Positions[Positions.Num() - 2]).GetSafeNormal() : FVector::ZeroVector;
}

void UChopItRopeComponent::ResetRope()
{
	Positions.Reset(); PreviousPositions.Reset(); RestLengths.Reset();
	InverseMasses.Reset(); Multipliers.Reset(); Contacts.Reset();
	ContactHeads.Reset(); ContactNext.Reset();
	CollisionBounds.Reset(); bCollisionBoundsReady = false;
	CollisionPlanes.Reset();
	DeployedLength = RequestedLength = AccumulatedTime = ReelVelocity = EndpointTension = 0;
	LastConstraintStep = StepTime;
	RejectedSteps = 0;
	TakeUpScale = 1.0f;
	SimulationDeadline = 0.0;
	BudgetLimitedFrames = 0;
	ResizeRejects = EndpointRejects = ConstraintRejects = 0;
	bInitialized = bMovementBlocked = false;
	bFrameBudgetLimited = false;
	bCinematicRetraction = false;
}
