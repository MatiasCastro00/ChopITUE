#pragma once

#include "CoreMinimal.h"
#include "ChopItTerrainPreviewMode.generated.h"

UENUM()
enum class EChopItTerrainPreviewMode : uint8
{
	Surface,
	Height UMETA(DisplayName="Height Map"),
	Slope UMETA(DisplayName="Slope Map"),
	Curvature UMETA(DisplayName="Curvature Map"),
	Walkability UMETA(DisplayName="Walkability Mask"),
	Clearing UMETA(DisplayName="Clearing Mask"),
	Moisture UMETA(DisplayName="Moisture Potential"),
	VegetationDensity UMETA(DisplayName="Vegetation Density Potential"),
	Regions UMETA(DisplayName="Region Map"),
	RegionBlend UMETA(DisplayName="Smooth Region Transitions"),
	WaterLevel UMETA(DisplayName="Water Level / Submerged Surface")
};
