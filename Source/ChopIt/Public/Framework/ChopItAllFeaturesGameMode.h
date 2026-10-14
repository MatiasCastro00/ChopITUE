#pragma once

#include "Framework/ChopItGameMode.h"
#include "ChopItAllFeaturesGameMode.generated.h"

class AChopItTerrainActor;
class AChopItMysteryPhone;
class UChopItPhoneSpawnConfig;

/** Full gameplay test: one fresh terrain run plus ground-aligned authored features. */
UCLASS()
class CHOPIT_API AChopItAllFeaturesGameMode : public AChopItGameMode
{
	GENERATED_BODY()
public:
	AChopItAllFeaturesGameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	/** Temporarily disabled on maps; opt in only for a dedicated terrain test. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Terrain")
	bool bGenerateTerrainAtRuntime = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Terrain")
	int32 TerrainRunSeed = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Terrain")
	TObjectPtr<AChopItTerrainActor> GeneratedTerrain;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Phone", meta=(ToolTip="Per-run phone spawn recipe. Missing assets disable the event."))
	TSoftObjectPtr<UChopItPhoneSpawnConfig> PhoneSpawnConfig;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Phone")
	TArray<TObjectPtr<AChopItMysteryPhone>> SpawnedPhones;

private:
	void SpawnMysteryPhones();
};
