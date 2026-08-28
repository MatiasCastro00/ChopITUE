#pragma once
#include "Components/ActorComponent.h"
#include "ChopItPactComponent.generated.h"
class UChopItPactDefinition;
class UChopItCombatStatsComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FChopItPactOffersChanged);
UCLASS(ClassGroup=(ChopIt), meta=(BlueprintSpawnableComponent))
class CHOPITCOMBAT_API UChopItPactComponent final : public UActorComponent
{
	GENERATED_BODY()
public:
	bool OpenOffers(const TArray<UChopItPactDefinition*>& NewOffers);
	bool SelectOffer(int32 Index, UChopItCombatStatsComponent* Stats);
	bool HasActiveOffer() const { return ActiveOffers.Num()>0; }
	int32 GetCurse() const { return Curse; }
	const TArray<TObjectPtr<UChopItPactDefinition>>& GetActiveOffers() const { return ActiveOffers; }
	UPROPERTY(BlueprintAssignable) FChopItPactOffersChanged OnOffersChanged;
private:
	UPROPERTY() TArray<TObjectPtr<UChopItPactDefinition>> ActiveOffers;
	UPROPERTY() TSet<FName> AcceptedIds;
	int32 Curse=0;
};
