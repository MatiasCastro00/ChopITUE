#include "Events/ChopItSpecialEventPlacement.h"

#include "Engine/World.h"
#include "Terrain/ChopItTerrainActor.h"

bool FChopItSpecialEventPlacementRules::IsValid() const
{
	return FMath::IsFinite(MinDistanceFromStart) && MinDistanceFromStart >= 0.0f
		&& FMath::IsFinite(MaxDistanceFromStart) && MaxDistanceFromStart >= MinDistanceFromStart
		&& FMath::IsFinite(MinDistanceBetweenEvents) && MinDistanceBetweenEvents >= 0.0f
		&& FMath::IsFinite(MaxSlopeDegrees) && MaxSlopeDegrees >= 0.0f && MaxSlopeDegrees < 90.0f
		&& FMath::IsFinite(MinWalkability) && MinWalkability >= 0.0f && MinWalkability <= 1.0f
		&& FMath::IsFinite(HeightAboveWater) && HeightAboveWater >= 0.0f
		&& FMath::IsFinite(FootprintRadius) && FootprintRadius > 0.0f
		&& FMath::IsFinite(ClearanceHalfHeight) && ClearanceHalfHeight >= FootprintRadius
		&& FMath::IsFinite(RouteSampleStep) && RouteSampleStep >= 0.0f
		&& FMath::IsFinite(RouteClearanceRadius) && RouteClearanceRadius > 0.0f
		&& FMath::IsFinite(RouteClearanceHalfHeight) && RouteClearanceHalfHeight >= RouteClearanceRadius
		&& MaxAttempts > 0;
}

namespace
{
bool IsUsableSurface(const AChopItTerrainActor& Terrain, const FVector& XY,
	const FChopItSpecialEventPlacementRules& Rules, FVector& OutGround)
{
	float LocalHeight = 0.0f;
	float WorldHeight = 0.0f;
	float Walkability = 0.0f;
	FVector Normal;
	if (!Terrain.SampleWorldMap(EChopItTerrainMap::Height, XY, LocalHeight)
		|| !Terrain.SampleWorldHeight(XY, WorldHeight)
		|| !Terrain.SampleWorldMap(EChopItTerrainMap::Walkability, XY, Walkability)
		|| !Terrain.SampleWorldNormal(XY, Normal)) return false;
	// The generated clearing is the playable area around PlayerStart. A terrain
	// recipe with automatic water can mark this deliberately flattened area as
	// submerged and give it zero walkability, even though it is the spawn pad.
	const FVector2D OffsetFromClearing(XY.X - Terrain.TerrainSettings.SpawnFlatCenter.X,
		XY.Y - Terrain.TerrainSettings.SpawnFlatCenter.Y);
	const bool bInSpawnClearing = Terrain.TerrainSettings.SpawnFlatRadius > 0.0f
		&& OffsetFromClearing.Size() + Rules.FootprintRadius <= Terrain.TerrainSettings.SpawnFlatRadius;
	if ((!bInSpawnClearing && (LocalHeight < Terrain.GetHeightField().GetWaterLevel() + Rules.HeightAboveWater
		|| Walkability < Rules.MinWalkability))
		|| FVector::DotProduct(Normal, FVector::UpVector) < FMath::Cos(FMath::DegreesToRadians(Rules.MaxSlopeDegrees))) return false;
	OutGround = FVector(XY.X, XY.Y, WorldHeight);
	return true;
}

bool HasUsableFootprint(const AChopItTerrainActor& Terrain, const FVector& Ground,
	const FChopItSpecialEventPlacementRules& Rules)
{
	const float Offset = Rules.FootprintRadius;
	const FVector2D Directions[] = {
		FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, 1), FVector2D(0, -1),
		FVector2D(0.70710678, 0.70710678), FVector2D(-0.70710678, 0.70710678),
		FVector2D(0.70710678, -0.70710678), FVector2D(-0.70710678, -0.70710678)
	};
	for (const FVector2D& Direction : Directions)
	{
		FVector Sample;
		if (!IsUsableSurface(Terrain, Ground + FVector(Direction.X * Offset, Direction.Y * Offset, 0), Rules, Sample)
			|| FMath::Abs(Sample.Z - Ground.Z) > Offset * FMath::Tan(FMath::DegreesToRadians(Rules.MaxSlopeDegrees))) return false;
	}
	return true;
}

bool HasUsableRoute(UWorld& World, const AChopItTerrainActor& Terrain, const FVector& Start, const FVector& Goal,
	const FChopItSpecialEventPlacementRules& Rules)
{
	if (Rules.RouteSampleStep <= 0.0f) return true;
	const int32 Steps = FMath::Max(1, FMath::CeilToInt(FVector::Dist2D(Start, Goal) / Rules.RouteSampleStep));
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ChopItSpecialEventRoute), false);
	Query.AddIgnoredActor(&Terrain);
	const FCollisionShape Clearance = FCollisionShape::MakeCapsule(
		Rules.RouteClearanceRadius, Rules.RouteClearanceHalfHeight);
	for (int32 Index = 1; Index < Steps; ++Index)
	{
		FVector Ground;
		if (!IsUsableSurface(Terrain, FMath::Lerp(Start, Goal, float(Index) / Steps), Rules, Ground)
			|| World.OverlapBlockingTestByChannel(Ground + FVector::UpVector * Rules.RouteClearanceHalfHeight,
				FQuat::Identity, ECC_Pawn, Clearance, Query)) return false;
	}
	return true;
}
}

bool FChopItSpecialEventPlacement::FindLocation(UWorld& World, const AChopItTerrainActor& Terrain,
	const FVector& Start, const FChopItSpecialEventPlacementRules& Rules, FRandomStream& Random,
	const TArray<FVector>& ExistingLocations, FVector& OutLocation)
{
	OutLocation = FVector::ZeroVector;
	if (!Rules.IsValid() || !Terrain.GetHeightField().IsValid()) return false;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ChopItSpecialEventPlacement), false);
	Query.AddIgnoredActor(&Terrain);
	const FCollisionShape Clearance = FCollisionShape::MakeCapsule(Rules.FootprintRadius, Rules.ClearanceHalfHeight);
	const float MinSquared = FMath::Square(Rules.MinDistanceBetweenEvents);
	for (int32 Attempt = 0; Attempt < Rules.MaxAttempts; ++Attempt)
	{
		const float Angle = Random.FRandRange(0.0f, 2.0f * UE_PI);
		const float RadiusSquared = Random.FRandRange(
			FMath::Square(Rules.MinDistanceFromStart), FMath::Square(Rules.MaxDistanceFromStart));
		const float Radius = FMath::Sqrt(RadiusSquared);
		const FVector XY = Start + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
		FVector Ground;
		if (!IsUsableSurface(Terrain, XY, Rules, Ground) || !HasUsableFootprint(Terrain, Ground, Rules)
			|| !HasUsableRoute(World, Terrain, Start, Ground, Rules)) continue;
		if (ExistingLocations.ContainsByPredicate([&](const FVector& Other)
			{ return FVector::DistSquared2D(Other, Ground) < MinSquared; })) continue;
		const FVector ClearanceCenter = Ground + FVector::UpVector * Rules.ClearanceHalfHeight;
		if (World.OverlapBlockingTestByChannel(ClearanceCenter, FQuat::Identity, ECC_Pawn, Clearance, Query)) continue;
		OutLocation = Ground;
		return true;
	}
	return false;
}
