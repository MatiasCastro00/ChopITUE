#pragma once

#include "CoreMinimal.h"
#include "ChopItSpecialEventPlacement.generated.h"

class AChopItTerrainActor;
class UWorld;

/** Geometry rules shared by one-off world events. Distances are in centimeters. */
USTRUCT(BlueprintType)
struct CHOPITWORLD_API FChopItSpecialEventPlacementRules
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Distance", meta=(ClampMin="0", Units="cm"))
	float MinDistanceFromStart = 1800.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Distance", meta=(ClampMin="0", Units="cm"))
	float MaxDistanceFromStart = 5000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Distance", meta=(ClampMin="0", Units="cm"))
	float MinDistanceBetweenEvents = 800.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Terrain", meta=(ClampMin="0", ClampMax="89", Units="deg"))
	float MaxSlopeDegrees = 18.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Terrain", meta=(ClampMin="0", ClampMax="1"))
	float MinWalkability = 0.55f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Terrain", meta=(ClampMin="0", Units="cm"))
	float HeightAboveWater = 40.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clearance", meta=(ClampMin="1", Units="cm"))
	float FootprintRadius = 110.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clearance", meta=(ClampMin="1", Units="cm"))
	float ClearanceHalfHeight = 120.0f;
	/** Sample the terrain between the player start and candidate; zero disables this check. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Access", meta=(ClampMin="0", Units="cm"))
	float RouteSampleStep = 250.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Access", meta=(ClampMin="1", Units="cm"))
	float RouteClearanceRadius = 55.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Access", meta=(ClampMin="1", Units="cm"))
	float RouteClearanceHalfHeight = 90.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Search", meta=(ClampMin="1", ClampMax="1000"))
	int32 MaxAttempts = 64;

	bool IsValid() const;
};

/** Seeded search independent of any event actor. Run only after terrain and authored obstacles are ready. */
class CHOPITWORLD_API FChopItSpecialEventPlacement
{
public:
	static bool FindLocation(UWorld& World, const AChopItTerrainActor& Terrain, const FVector& Start,
		const FChopItSpecialEventPlacementRules& Rules, FRandomStream& Random,
		const TArray<FVector>& ExistingLocations, FVector& OutLocation);
};
