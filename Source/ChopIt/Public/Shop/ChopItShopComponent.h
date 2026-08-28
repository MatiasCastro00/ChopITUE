#pragma once

#include "Components/ActorComponent.h"
#include "ChopItShopComponent.generated.h"

class UChopItWeaponDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FChopItShopOffersChanged);

/** Owns a transient shop session; payment and loadout changes are performed as one rollback-safe operation. */
UCLASS(ClassGroup = (ChopIt), BlueprintType, meta = (BlueprintSpawnableComponent))
class CHOPIT_API UChopItShopComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItShopComponent();
	void OpenShop(const TArray<UChopItWeaponDefinition*>& NewOffers);
	void CloseShop();
	bool HasActiveShop() const { return ActiveOffers.Num() > 0; }
	const TArray<TObjectPtr<UChopItWeaponDefinition>>& GetActiveOffers() const { return ActiveOffers; }
	bool SelectOffer(int32 Index, AActor* Purchaser, FString* OutFailureReason = nullptr);

	UPROPERTY(BlueprintAssignable, Category = "ChopIt|Shop")
	FChopItShopOffersChanged OnOffersChanged;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UChopItWeaponDefinition>> ActiveOffers;
};
