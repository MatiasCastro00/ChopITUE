#pragma once

#include "Engine/DeveloperSettings.h"
#include "Terrain/ChopItTerrainDataAsset.h"
#include "ChopItTerrainProjectSettings.generated.h"

/** The project-wide terrain recipe. Seeds belong to individual runs. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="ChopIt Terrain"))
class CHOPITWORLD_API UChopItTerrainProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Generation",
		meta=(ToolTip="Terrain recipe used by worlds that generate the project default terrain."))
	TSoftObjectPtr<UChopItTerrainDataAsset> DefaultTerrain;
};
