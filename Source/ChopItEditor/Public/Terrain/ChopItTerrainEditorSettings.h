#pragma once

#include "CoreMinimal.h"
#include "Terrain/ChopItTerrainSettings.h"
#include "Terrain/ChopItTerrainMeshSettings.h"
#include "Terrain/ChopItTerrainPreviewMode.h"
#include "Terrain/ChopItTerrainMaps.h"
#include "Terrain/ChopItTerrainPresets.h"
#include "ChopItTerrainEditorSettings.generated.h"

/** Per-user settings shown by the terrain editor tab. Editing does not change a level until Generate is pressed. */
UCLASS(Config=EditorPerProjectUserSettings)
class CHOPITEDITOR_API UChopItTerrainEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Config, Category="Generation", meta=(ShowOnlyInnerProperties, ToolTip="Settings used to generate the source height field and spatial maps."))
	FChopItTerrainSettings Terrain;

	UPROPERTY(EditAnywhere, Config, Category="Geometry", meta=(ShowOnlyInnerProperties, ToolTip="Polygon detail, shading, chunks and collision for the final terrain."))
	FChopItTerrainMeshSettings Mesh;

	UPROPERTY(EditAnywhere, Config, Category="Preview", meta=(ClampMin="0", ClampMax="100", UIMin="0", UIMax="100", ToolTip="Polygon detail for the preview only. The source heightmap stays identical to the generated level terrain."))
	int32 PreviewPolygonDetail = 35;

	UPROPERTY(EditAnywhere, Config, Category="Preview", meta=(ToolTip="Display a spatial map on the preview terrain. Colors sample the source grid; increasing preview detail resolves finer map features."))
	EChopItTerrainPreviewMode PreviewMap = EChopItTerrainPreviewMode::Surface;

	UPROPERTY(EditAnywhere, Config, Category="Mask Editor", meta=(ToolTip="Spatial mask affected by the paint, erase and smooth brushes."))
	EChopItEditableMask EditMask = EChopItEditableMask::Clearing;
	/** World position in centimeters. Set it from the selected terrain actor or the viewport cursor. */
	UPROPERTY(EditAnywhere, Config, Category="Mask Editor", meta=(ToolTip="World position at which the next mask brush stroke will be applied."))
	FVector BrushWorldPosition = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Config, Category="Mask Editor", meta=(ClampMin="1", UIMin="100", UIMax="5000", Units="cm", ToolTip="World-space brush radius in centimeters."))
	float BrushRadius = 1000.0f;
	UPROPERTY(EditAnywhere, Config, Category="Mask Editor", meta=(ClampMin="0", ClampMax="1", ToolTip="Brush blend strength from no effect to full target value."))
	float BrushStrength = 1.0f;
	UPROPERTY(EditAnywhere, Config, Category="Mask Editor", meta=(ClampMin="0", ClampMax="1", ToolTip="Mask value painted at full brush strength."))
	float BrushTargetValue = 1.0f;
	UPROPERTY(EditAnywhere, Config, Category="Mask Editor", meta=(ToolTip="Click once on the terrain preview to apply the chosen brush. Camera orbit remains on right drag."))
	bool bPaintOnPreview = false;
	UPROPERTY(EditAnywhere, Config, Category="Mask Editor", meta=(ToolTip="Paint, erase or smooth when clicking the terrain preview."))
	EChopItMaskBrush PreviewBrush = EChopItMaskBrush::Paint;

	UPROPERTY(EditAnywhere, Config, Category="Data", meta=(ToolTip="File path used to save or load a complete terrain snapshot."))
	FFilePath TerrainDataFile;
	UPROPERTY(EditAnywhere, Config, Category="Data", meta=(ToolTip="Folder for exported height and mask images."))
	FDirectoryPath ExportDirectory;

	UPROPERTY(EditAnywhere, Config, Category="Presets", meta=(ToolTip="Curated forest terrain style. Apply Preset updates the editor controls while keeping the current seed and dimensions."))
	EChopItTerrainPreset SelectedPreset = EChopItTerrainPreset::FlatForest;
	UPROPERTY(EditAnywhere, Config, Category="Presets", meta=(ToolTip="JSON file used by Save Preset and Load Preset."))
	FFilePath PresetFile;

	UPROPERTY(EditAnywhere, Config, Category="Parameter Tools", meta=(GetOptions="GetCategoryOptions", ToolTip="Category to randomize. All affects every unlocked numeric or boolean control."))
	FString SelectedCategory = TEXT("Relief");
	UPROPERTY(EditAnywhere, Config, Category="Parameter Tools", meta=(GetOptions="GetParameterOptions", ToolTip="Parameter used by Lock/Unlock and Reset Parameter."))
	FString SelectedParameter = TEXT("Terrain.HillStrength");
	UPROPERTY(VisibleAnywhere, Config, Category="Parameter Tools", meta=(ToolTip="Bulk actions, presets and Reset Defaults preserve these values."))
	TArray<FString> LockedParameters;

	UFUNCTION()
	TArray<FString> GetCategoryOptions() const;
	UFUNCTION()
	TArray<FString> GetParameterOptions() const;
};
