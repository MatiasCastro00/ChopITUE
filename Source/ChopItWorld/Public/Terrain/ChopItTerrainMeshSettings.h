#pragma once

#include "CoreMinimal.h"
#include "ChopItTerrainMeshSettings.generated.h"

UENUM(BlueprintType)
enum class EChopItTerrainShading : uint8
{
	Smooth,
	Flat
};

USTRUCT(BlueprintType)
struct CHOPITWORLD_API FChopItTerrainMeshSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Geometry", meta = (ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100", ToolTip = "8 to 256 cells along the longest side. Resamples the same heightmap without changing its data."))
	int32 PolygonDetail = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Geometry", meta=(ToolTip="Smooth shares vertex normals across faces; Flat gives each face its own normal without changing the heightmap."))
	EChopItTerrainShading Shading = EChopItTerrainShading::Smooth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Geometry", meta = (ClampMin = "8", ClampMax = "128", UIMin = "8", UIMax = "128", ToolTip = "Maximum cells per chunk side. Changing this partition does not change the surface."))
	int32 CellsPerChunk = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Collision", meta = (ToolTip = "Static triangle collision for characters and physics objects. The terrain itself is not simulated."))
	bool bEnableCollision = true;
};
