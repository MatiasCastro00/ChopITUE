#pragma once

#include "Components/ActorComponent.h"
#include "ChopItUpgradeOfferComponent.generated.h"

class UChopItCombatStatsComponent;
class UChopItExperienceComponent;
class UChopItUpgradeDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FChopItUpgradeOffersChanged);

/** Deterministic, weighted, without-replacement level-up offers and reversible modifier handles. */
UCLASS(ClassGroup = (ChopIt), BlueprintType, meta = (BlueprintSpawnableComponent))
class CHOPITCOMBAT_API UChopItUpgradeOfferComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItUpgradeOfferComponent();
	virtual void BeginPlay() override;

	bool SelectOffer(int32 OfferIndex);
	bool ApplyUpgrade(UChopItUpgradeDefinition* Upgrade, UChopItCombatStatsComponent* Stats);
	bool RemoveLastStack(FName UpgradeId, UChopItCombatStatsComponent* Stats);
	bool HasActiveOffer() const { return ActiveOffers.Num() > 0; }
	const TArray<TObjectPtr<UChopItUpgradeDefinition>>& GetActiveOffers() const { return ActiveOffers; }
	int32 GetStackCount(FName UpgradeId) const;

	UPROPERTY(BlueprintAssignable, Category = "ChopIt|Progression")
	FChopItUpgradeOffersChanged OnOffersChanged;

#if WITH_DEV_AUTOMATION_TESTS
	void SetCatalogForAutomation(const TArray<UChopItUpgradeDefinition*>& Definitions);
	void GenerateOffersForAutomation(int32 Level, int32 Seed);
#endif

private:
	UFUNCTION()
	void HandleLevelUpQueued(int32 NewLevel, int32 PendingLevelUps);

	void LoadDefaultCatalog();
	void BuildOffers(int32 Level, int32 Seed);
	void SetPaused(bool bPaused) const;
	UChopItCombatStatsComponent* ResolveStats() const;
	UChopItExperienceComponent* ResolveExperience() const;

	UPROPERTY(EditDefaultsOnly, Category = "ChopIt|Progression")
	int32 OfferSeed = 1337;

	UPROPERTY(EditDefaultsOnly, Category = "ChopIt|Progression", meta = (ClampMin = "1", ClampMax = "5"))
	int32 OfferCount = 3;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UChopItUpgradeDefinition>> Catalog;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UChopItUpgradeDefinition>> ActiveOffers;

	TMap<FName, int32> StackCounts;
	TMap<FName, TArray<FGuid>> AppliedHandles;
	int32 OfferSequence = 0;
	bool bSelectionLocked = false;
};
