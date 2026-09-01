#include "Weapons/ChopItWeaponLoadoutComponent.h"

#include "Weapons/ChopItAutoAttackComponent.h"
#include "Weapons/ChopItWeaponDefinition.h"

UChopItWeaponLoadoutComponent::UChopItWeaponLoadoutComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UChopItWeaponLoadoutComponent::CanEquipWeapon(const UChopItWeaponDefinition* Weapon, FString* OutFailureReason) const
{
	auto Fail = [OutFailureReason](const TCHAR* Reason)
	{
		if (OutFailureReason) { *OutFailureReason = Reason; }
		return false;
	};
	if (!Weapon || Weapon->WeaponId.IsNone()) { return Fail(TEXT("Invalid weapon")); }
	if (Weapon->bExclusiveToStartingCharacter) { return Fail(TEXT("Exclusive weapon")); }
	if (!Weapon->bUsesLoadoutSlot) { return Fail(TEXT("Weapon cannot be equipped")); }
	if (EquippedWeapons.Num() >= SlotCount) { return Fail(TEXT("All slots are full")); }
	for (const UChopItWeaponDefinition* Equipped : EquippedWeapons)
	{
		if (Equipped && Equipped->WeaponId == Weapon->WeaponId) { return Fail(TEXT("Weapon already equipped")); }
	}
	return true;
}

bool UChopItWeaponLoadoutComponent::TryEquipWeapon(UChopItWeaponDefinition* Weapon, FString* OutFailureReason)
{
	if (!CanEquipWeapon(Weapon, OutFailureReason) || !GetOwner())
	{
		return false;
	}
	const FName ComponentName(*FString::Printf(TEXT("AdditionalWeapon_%s"), *Weapon->WeaponId.ToString()));
	UChopItAutoAttackComponent* AttackComponent = NewObject<UChopItAutoAttackComponent>(GetOwner(), ComponentName);
	if (!AttackComponent) { return false; }
	AttackComponent->RegisterComponent();
	AttackComponent->SetWeaponDefinition(Weapon);
	EquippedWeapons.Add(Weapon);
	EquippedAttackComponents.Add(Weapon->WeaponId, AttackComponent);
	return true;
}

bool UChopItWeaponLoadoutComponent::RemoveWeapon(const FName WeaponId)
{
	const int32 Index = EquippedWeapons.IndexOfByPredicate([WeaponId](const UChopItWeaponDefinition* Weapon)
	{
		return Weapon && Weapon->WeaponId == WeaponId;
	});
	if (Index == INDEX_NONE) { return false; }
	if (UChopItAutoAttackComponent* AttackComponent = EquippedAttackComponents.FindRef(WeaponId))
	{
		AttackComponent->DestroyComponent();
	}
	EquippedAttackComponents.Remove(WeaponId);
	EquippedWeapons.RemoveAt(Index);
	return true;
}
