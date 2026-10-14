#include "Terrain/ChopItTerrainEditorSettings.h"
#include "Terrain/ChopItTerrainParameterTools.h"

TArray<FString> UChopItTerrainEditorSettings::GetCategoryOptions() const
{
	return FChopItTerrainParameterTools::GetCategories();
}

TArray<FString> UChopItTerrainEditorSettings::GetParameterOptions() const
{
	return FChopItTerrainParameterTools::GetParameterPaths();
}
