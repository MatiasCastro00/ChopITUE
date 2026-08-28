#pragma once

#include "Commandlets/Commandlet.h"
#include "ChopItBootstrapCommandlet.generated.h"

/** Creates deterministic bootstrap assets and the current phase's development maps. */
UCLASS()
class CHOPITEDITOR_API UChopItBootstrapCommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UChopItBootstrapCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	bool CreateMapIfMissing(const FString& LongPackageName) const;
	bool CreatePhase1Assets() const;
	bool CreatePhase2Assets() const;
	bool CreatePhase3Assets() const;
	bool CreatePhase4Assets() const;
	bool CreatePhase5Assets() const;
	bool CreatePhase6Assets() const;
	bool CreatePhase7Assets() const;
	bool CreatePhase8Assets() const;
	bool CreatePhase9Assets() const;
	bool CreatePhase10Assets() const;
	bool CreatePhase12Assets() const;
	bool CreateEnemyAssets() const;
	bool CreateProgressionAssets() const;
	bool CreateInputAssets() const;
	bool CreateCharacterBlueprint() const;
	bool CreateBasicAxeAsset() const;
	bool CreateSharedWeaponAssets() const;
	bool CreateShopBlueprint() const;
	bool CreateHarvestBlueprints() const;
	bool CreateEconomyBlueprints() const;
	bool CreateDayDefinition() const;
	bool CreateBlockoutMaterials() const;
	bool RebuildPhase1Map(
		const FString& LongPackageName,
		bool bFullSandbox,
		bool bIncludeCombatDummies = false,
		bool bHarvestTestLayout = false,
		bool bEconomyTestLayout = false) const;
};
