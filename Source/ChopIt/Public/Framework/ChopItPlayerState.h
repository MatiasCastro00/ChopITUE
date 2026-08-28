#pragma once

#include "GameFramework/PlayerState.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "ChopItPlayerState.generated.h"

class UChopItEconomyComponent;
class UChopItExperienceComponent;
class UChopItUpgradeOfferComponent;
class UChopItShopComponent;
class UChopItPactComponent;

/** Run-scoped player state; economy and progression components arrive in later phases. */
UCLASS()
class CHOPIT_API AChopItPlayerState final : public APlayerState
{
	GENERATED_BODY()

public:
	AChopItPlayerState();
	UChopItEconomyComponent* GetEconomyComponent() const { return EconomyComponent; }
	UChopItExperienceComponent* GetExperienceComponent() const { return ExperienceComponent; }
	UChopItUpgradeOfferComponent* GetUpgradeOfferComponent() const { return UpgradeOfferComponent; }
	UChopItShopComponent* GetShopComponent() const { return ShopComponent; }
	UChopItPactComponent* GetPactComponent() const { return PactComponent; }
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Economy")
	TObjectPtr<UChopItEconomyComponent> EconomyComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Progression")
	TObjectPtr<UChopItExperienceComponent> ExperienceComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Progression")
	TObjectPtr<UChopItUpgradeOfferComponent> UpgradeOfferComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Shop")
	TObjectPtr<UChopItShopComponent> ShopComponent;
	UPROPERTY(VisibleAnywhere, Category="ChopIt|Pacts") TObjectPtr<UChopItPactComponent> PactComponent;
	UFUNCTION() void HandlePhase(EChopItCyclePhase NewPhase, EChopItCyclePhase PreviousPhase, int32 Generation);
};
