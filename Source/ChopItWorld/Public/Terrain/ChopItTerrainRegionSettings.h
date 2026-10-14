#pragma once

#include "CoreMinimal.h"
#include "ChopItTerrainRegionSettings.generated.h"

/** Classification controls; they describe the generated surface without changing its geometry. */
USTRUCT(BlueprintType)
struct CHOPITWORLD_API FChopItTerrainRegionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Regions", meta=(ClampMin="100", ClampMax="10000000", Units="cm", ToolTip="Horizontal scale of clearing patches and broad valley detection."))
	double RegionScale = 6000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Regions", meta=(ClampMin="0", ClampMax="1", ToolTip="Clearing selection coverage before slope and water suitability; 0 disables clearings, 1 selects all suitable ground."))
	float ClearingCoverage = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Regions", meta=(ClampMin="0.01", ClampMax="0.45", ToolTip="Soft transition width for region selection and slope thresholds."))
	float TransitionWidth = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Regions", meta=(ClampMin="0.1", ClampMax="85", Units="deg", ToolTip="Slope below which land belongs mainly to the flat forest region."))
	float FlatSlopeDegrees = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Regions", meta=(ClampMin="1", ClampMax="89", Units="deg", ToolTip="Must exceed FlatSlopeDegrees."))
	float SteepSlopeDegrees = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Suitability", meta=(ClampMin="1", ClampMax="89", Units="deg", ToolTip="Slope threshold for the soft walkability mask. This is terrain suitability, not a navigation guarantee."))
	float MaxWalkableSlopeDegrees = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Suitability", meta=(ClampMin="1", ClampMax="100000", Units="cm", ToolTip="Height above water over which dry-ground suitability fades in."))
	float ShoreBlendHeight = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Suitability", meta=(ClampMin="1", ClampMax="100000", Units="cm", ToolTip="Vertical distance over which water proximity contributes to moisture potential."))
	float MoistureHeightRange = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Suitability", meta=(ClampMin="0", ClampMax="1", ToolTip="Global multiplier for procedural vegetation density potential."))
	float VegetationDensity = 0.8f;

	bool Validate(FString& OutError) const;
};
