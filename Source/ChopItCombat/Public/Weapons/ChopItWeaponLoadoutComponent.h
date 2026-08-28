#pragma once

#include "Components/ActorComponent.h"
#include "ChopItWeaponLoadoutComponent.generated.h"

class UChopItAutoAttackComponent;
class UChopItWeaponDefinition;

/** Owns run-scoped shared weapons. The starting exclusive weapon is intentionally not a slot. */
UCLASS(ClassGroup = (ChopIt), BlueprintType, meta = (BlueprintSpawnableComponent))
class CHOPITCOMBAT_API UChopItWeaponLoadoutComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItWeaponLoadoutComponent();
	bool CanEquipWeapon(const UChopItWeaponDefinition* Weapon, FString* OutFailureReason = nullptr) const;
	bool TryEquipWeapon(UChopItWeaponDefinition* Weapon, FString* OutFailureReason = nullptr);
	bool RemoveWeapon(FName WeaponId);
	int32 GetUsedSlots() const { return EquippedWeapons.Num(); }
	int32 GetSlotCount() const { return SlotCount; }
	const TArray<TObjectPtr<UChopItWeaponDefinition>>& GetEquippedWeapons() const { return EquippedWeapons; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "ChopIt|Loadout", meta = (ClampMin = "0", ClampMax = "8"))
	int32 SlotCount = 2;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Loadout")
	TArray<TObjectPtr<UChopItWeaponDefinition>> EquippedWeapons;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UChopItAutoAttackComponent>> EquippedAttackComponents;
};
