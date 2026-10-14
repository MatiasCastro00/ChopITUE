#pragma once

#include "Terrain/ChopItHeightField.h"
#include "Terrain/ChopItTerrainSettings.h"

/** Stateless CPU generation. No world, actor, editor or global random state is required. */
class CHOPITWORLD_API FChopItTerrainGenerator
{
public:
	static constexpr int32 GeneratorVersion = 4;

	/** Invalid settings leave OutField unchanged and provide a diagnostic. */
	static bool Generate(const FChopItTerrainSettings& Settings, FChopItHeightField& OutField, FString& OutError);
};
