#pragma once

#include "DynamicMesh/DynamicMesh3.h"
#include "Terrain/ChopItHeightField.h"
#include "Terrain/ChopItTerrainMeshSettings.h"

struct CHOPITWORLD_API FChopItTerrainMeshChunk
{
	FIntPoint FirstCell = FIntPoint::ZeroValue;
	FIntPoint CellCount = FIntPoint::ZeroValue;
	// The mesh owns attributes with back-pointers: keep its address stable when the chunk array grows.
	TUniquePtr<UE::Geometry::FDynamicMesh3> Mesh = MakeUnique<UE::Geometry::FDynamicMesh3>();
};

/** Pure mesh data builder. All chunks use the same centered terrain-local coordinate system. */
class CHOPITWORLD_API FChopItTerrainMeshBuilder
{
public:
	static FIntPoint GetCellCounts(const FVector2D& Size, int32 PolygonDetail);
	/** Failure preserves OutChunks. Normals and UVs are continuous across chunk boundaries. */
	static bool Build(const FChopItHeightField& Field, const FChopItTerrainMeshSettings& Settings,
		TArray<FChopItTerrainMeshChunk>& OutChunks, FString& OutError);
};
