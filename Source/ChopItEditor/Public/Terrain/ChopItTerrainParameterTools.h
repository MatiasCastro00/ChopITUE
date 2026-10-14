#pragma once

#include "CoreMinimal.h"
#include "Terrain/ChopItTerrainSettings.h"
#include "Terrain/ChopItTerrainMeshSettings.h"

class UChopItTerrainEditorSettings;

/** Reflected parameter actions used by the editor tab; all bulk edits respect locked paths. */
class CHOPITEDITOR_API FChopItTerrainParameterTools
{
public:
	static TArray<FString> GetParameterPaths();
	static TArray<FString> GetCategories();
	static bool IsLocked(const UChopItTerrainEditorSettings& Settings, const FString& Path);
	static bool IsDefault(const UChopItTerrainEditorSettings& Settings, const FString& Path);
	static bool ToggleLock(UChopItTerrainEditorSettings& Settings, const FString& Path, FString& OutError);
	static bool ResetParameter(UChopItTerrainEditorSettings& Settings, const FString& Path, FString& OutError);
	static bool Randomize(UChopItTerrainEditorSettings& Settings, const FString& Category, FString& OutError);
	static bool ApplyUnlocked(UChopItTerrainEditorSettings& Settings, const FChopItTerrainSettings& Terrain,
		const FChopItTerrainMeshSettings& Mesh, FString& OutError);
};
