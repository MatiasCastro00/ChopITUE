#pragma once

#include "CoreMinimal.h"
#include "Terrain/ChopItTerrainSettings.h"
#include "Terrain/ChopItTerrainMeshSettings.h"
#include "ChopItTerrainPresets.generated.h"

UENUM()
enum class EChopItTerrainPreset : uint8
{
	FlatForest UMETA(DisplayName="Flat Forest"),
	GentleForestHills UMETA(DisplayName="Gentle Forest Hills"),
	RollingHills UMETA(DisplayName="Rolling Hills"),
	ForestValleys UMETA(DisplayName="Forest Valleys"),
	SteepWoodland UMETA(DisplayName="Steep Woodland"),
	WetLowlands UMETA(DisplayName="Wet Lowlands"),
	ForestClearings UMETA(DisplayName="Forest Clearings")
};

/** File format for user presets. Presets store controls, not generated samples or manual mask edits. */
USTRUCT()
struct FChopItTerrainPresetFile
{
	GENERATED_BODY()
	UPROPERTY() int32 Version = 1;
	UPROPERTY() FChopItTerrainSettings Terrain;
	UPROPERTY() FChopItTerrainMeshSettings Mesh;
};

class CHOPITEDITOR_API FChopItTerrainPresets
{
public:
	static bool Build(EChopItTerrainPreset Preset, const FChopItTerrainSettings& Current,
		FChopItTerrainSettings& OutTerrain, FChopItTerrainMeshSettings& OutMesh, FString& OutError);
	static bool Save(const FString& Path, const FChopItTerrainSettings& Terrain,
		const FChopItTerrainMeshSettings& Mesh, FString& OutError);
	static bool Load(const FString& Path, FChopItTerrainSettings& OutTerrain,
		FChopItTerrainMeshSettings& OutMesh, FString& OutError);
};
