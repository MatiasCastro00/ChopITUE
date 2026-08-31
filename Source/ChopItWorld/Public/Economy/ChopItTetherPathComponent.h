#pragma once

#include "Components/SceneComponent.h"
#include "ChopItTetherPathComponent.generated.h"

class UChopItChainDefinition;
class UPrimitiveComponent;

/** A persistent bend in the authoritative tether route. */
USTRUCT()
struct CHOPITWORLD_API FChopItWrapAnchor
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> Component;

	UPROPERTY(Transient)
	FVector LocalPosition = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector LocalNormal = FVector::UpVector;

	UPROPERTY(Transient)
	FVector FallbackWorldPosition = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector FallbackWorldNormal = FVector::UpVector;

	UPROPERTY(Transient)
	int32 StableId = INDEX_NONE;

	UPROPERTY(Transient)
	int32 ClearFrames = 0;

	FVector GetWorldPosition() const;
	FVector GetWorldNormal() const;
};

/**
 * Deterministic gameplay authority for a tether. It records an ordered polyline
 * from machine to player and is intentionally independent from visual particles.
 */
UCLASS(ClassGroup = (ChopIt), meta = (BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItTetherPathComponent final : public USceneComponent
{
	GENERATED_BODY()

public:
	UChopItTetherPathComponent();

	void Configure(const UChopItChainDefinition* Definition);
	void InitializePath(const FVector& StartWorld, const FVector& EndWorld, AActor* InIgnoredEndActor);
	void UpdatePath(const FVector& StartWorld, const FVector& EndWorld);
	void ResetPath();

	const TArray<FVector>& GetRoutePoints() const { return RoutePoints; }
	const TArray<int32>& GetRoutePointIds() const { return RoutePointIds; }
	const TArray<FChopItWrapAnchor>& GetAnchors() const { return Anchors; }
	float GetRouteLength() const { return RouteLength; }
	float GetPrefixLengthBeforeFinalSpan() const;
	FVector GetFinalGuidePoint() const;
	bool IsAtAnchorCapacity() const { return bAtAnchorCapacity; }
	int32 GetAnchorCount() const { return Anchors.Num(); }

	/** Pulls physics props from both neighbouring spans; never affects Pawn actors. */
	void ApplyTensionToPhysicsProps(float TensionAlpha, float MaximumForce, float MassScale) const;

	/** Pure helper used by tests and by the runtime cache. */
	static float CalculatePolylineLength(const TArray<FVector>& Points);

private:
	void RefreshRouteCache();
	void RemoveInvalidAnchors();
	void TryUnwrapAnchors();
	void TryInsertAnchors();
	bool SweepSegment(
		const FVector& Start,
		const FVector& End,
		FHitResult& OutHit,
		const UPrimitiveComponent* IgnoredComponent = nullptr) const;
	FChopItWrapAnchor MakeAnchor(const FHitResult& Hit);
	void DrawDebugRoute() const;

	UPROPERTY(Transient)
	TArray<FChopItWrapAnchor> Anchors;

	UPROPERTY(Transient)
	TArray<FVector> RoutePoints;

	UPROPERTY(Transient)
	TArray<int32> RoutePointIds;

	TWeakObjectPtr<AActor> IgnoredEndActor;
	FVector StartEndpoint = FVector::ZeroVector;
	FVector EndEndpoint = FVector::ZeroVector;
	float RouteLength = 0.0f;
	float SweepRadius = 8.0f;
	float SurfaceOffset = 1.5f;
	float MinimumAnchorSeparation = 12.0f;
	int32 MaximumAnchors = 32;
	int32 MaximumInsertionsPerFrame = 4;
	int32 UnwrapFrames = 3;
	int32 NextStableId = 1;
	bool bAtAnchorCapacity = false;
	bool bDebugDraw = false;
};
