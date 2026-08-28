#include "Shop/ChopItShopComponent.h"

#include "Economy/ChopItEconomyComponent.h"
#include "Weapons/ChopItWeaponDefinition.h"
#include "Weapons/ChopItWeaponLoadoutComponent.h"

UChopItShopComponent::UChopItShopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItShopComponent::OpenShop(const TArray<UChopItWeaponDefinition*>& NewOffers)
{
	ActiveOffers.Reset();
	for (UChopItWeaponDefinition* Offer : NewOffers)
	{
		if (Offer) { ActiveOffers.Add(Offer); }
	}
	OnOffersChanged.Broadcast();
}

void UChopItShopComponent::CloseShop()
{
	if (ActiveOffers.IsEmpty()) { return; }
	ActiveOffers.Reset();
	OnOffersChanged.Broadcast();
}

bool UChopItShopComponent::SelectOffer(const int32 Index, AActor* Purchaser, FString* OutFailureReason)
{
	auto Fail = [OutFailureReason](const TCHAR* Reason)
	{
		if (OutFailureReason) { *OutFailureReason = Reason; }
		return false;
	};
	if (!ActiveOffers.IsValidIndex(Index) || !Purchaser) { return Fail(TEXT("Oferta invalida")); }
	UChopItWeaponDefinition* Weapon = ActiveOffers[Index];
	UChopItEconomyComponent* Economy = GetOwner() ? GetOwner()->FindComponentByClass<UChopItEconomyComponent>() : nullptr;
	UChopItWeaponLoadoutComponent* Loadout = Purchaser->FindComponentByClass<UChopItWeaponLoadoutComponent>();
	if (!Weapon || !Economy || !Loadout) { return Fail(TEXT("Tienda no disponible")); }
	if (Economy->GetBalance() < Weapon->ShopPrice) { return Fail(TEXT("Dinero insuficiente")); }
	if (!Loadout->TryEquipWeapon(Weapon, OutFailureReason)) { return false; }
	if (!Economy->ApplyTransaction(FGuid::NewGuid(), TEXT("WeaponPurchase"), -Weapon->ShopPrice))
	{
		Loadout->RemoveWeapon(Weapon->WeaponId);
		return Fail(TEXT("No se pudo cobrar la compra"));
	}
	ActiveOffers.RemoveAt(Index);
	OnOffersChanged.Broadcast();
	return true;
}
