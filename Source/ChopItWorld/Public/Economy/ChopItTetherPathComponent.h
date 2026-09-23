#pragma once
#include "Components/SceneComponent.h"
#include "ChopItTetherPathComponent.generated.h"

class UChopItChainDefinition;
class UChopItRopeComponent;
class UPrimitiveComponent;

/** Compatibility/debug representation of a sliding solver contact. */
USTRUCT()
struct CHOPITWORLD_API FChopItWrapAnchor
{
	GENERATED_BODY()
	UPROPERTY(Transient) TWeakObjectPtr<UPrimitiveComponent> Component;
	UPROPERTY(Transient) FVector LocalPosition = FVector::ZeroVector;
	UPROPERTY(Transient) FVector LocalNormal = FVector::UpVector;
	UPROPERTY(Transient) FVector FallbackWorldPosition = FVector::ZeroVector;
	UPROPERTY(Transient) FVector FallbackWorldNormal = FVector::UpVector;
	UPROPERTY(Transient) int32 StableId = INDEX_NONE;
	FVector GetWorldPosition() const;
	FVector GetWorldNormal() const;
};

/** Read-only compatibility adapter. It never invents or shortens the physical path. */
UCLASS(ClassGroup = (ChopIt), meta = (BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItTetherPathComponent final : public USceneComponent
{
	GENERATED_BODY()
public:
	UChopItTetherPathComponent();
	void Configure(const UChopItChainDefinition* Definition);
	void SetSource(UChopItRopeComponent* InSource);
	void Synchronize();
	void ResetPath();
	const TArray<FVector>& GetRoutePoints() const { return RoutePoints; }
	const TArray<int32>& GetRoutePointIds() const { return RoutePointIds; }
	const TArray<FChopItWrapAnchor>& GetAnchors() const { return Anchors; }
	float GetRouteLength() const;
	float GetPrefixLengthBeforeFinalSpan() const;
	FVector GetFinalGuidePoint() const;
	bool IsAtAnchorCapacity() const { return false; }
	int32 GetAnchorCount() const { return Anchors.Num(); }
	static float CalculatePolylineLength(const TArray<FVector>& Points);
private:
	UPROPERTY(Transient) TWeakObjectPtr<UChopItRopeComponent> Source;
	UPROPERTY(Transient) TArray<FVector> RoutePoints;
	UPROPERTY(Transient) TArray<int32> RoutePointIds;
	UPROPERTY(Transient) TArray<FChopItWrapAnchor> Anchors;
	bool bDebugDraw = false;
};
