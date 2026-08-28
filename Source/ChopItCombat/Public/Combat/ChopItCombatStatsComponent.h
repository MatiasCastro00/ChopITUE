#pragma once

#include "Components/ActorComponent.h"
#include "ChopItCombatStatsComponent.generated.h"

UENUM(BlueprintType)
enum class EChopItCombatStat : uint8
{
	Damage,
	AttackSpeed,
	Range,
	CriticalChance,
	MovementSpeed
};

UENUM(BlueprintType)
enum class EChopItModifierOperation : uint8
{
	Add,
	Multiply
};

USTRUCT(BlueprintType)
struct CHOPITCOMBAT_API FChopItStatModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EChopItCombatStat Stat = EChopItCombatStat::Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EChopItModifierOperation Operation = EChopItModifierOperation::Add;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Magnitude = 0.0f;
};

UCLASS(ClassGroup = (ChopIt), meta = (BlueprintSpawnableComponent))
class CHOPITCOMBAT_API UChopItCombatStatsComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItCombatStatsComponent();

	FGuid AddModifier(const FChopItStatModifier& Modifier);
	bool RemoveModifier(const FGuid& Handle);
	float EvaluateStat(EChopItCombatStat Stat, float BaseValue) const;
	void ClearModifiers();
	int32 GetModifierCount() const { return Modifiers.Num(); }

	FSimpleMulticastDelegate OnStatsChanged;

private:
	TMap<FGuid, FChopItStatModifier> Modifiers;
};
