#pragma once

#include "Engine/DataAsset.h"
#include "ChopItWeaponDefinition.generated.h"

UENUM(BlueprintType)
enum class EChopItWeaponAttackPattern : uint8
{
	ArcMelee,
	RadialMelee
};

UCLASS(BlueprintType)
class CHOPITCOMBAT_API UChopItWeaponDefinition final : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName WeaponId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EChopItWeaponAttackPattern AttackPattern = EChopItWeaponAttackPattern::ArcMelee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Loadout")
	bool bExclusiveToStartingCharacter = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Loadout")
	bool bUsesLoadoutSlot = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Shop", meta = (ClampMin = "0"))
	int64 ShopPrice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float Damage = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.05"))
	float AttackInterval = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float Range = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1.0", ClampMax = "180.0"))
	float ArcHalfAngleDegrees = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1"))
	int32 MaxTargets = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CriticalChance = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1.0"))
	float CriticalMultiplier = 2.0f;
};
