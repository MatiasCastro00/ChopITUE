#pragma once

#include "Components/ActorComponent.h"
#include "ChopItAutoAttackComponent.generated.h"

class UChopItCombatStatsComponent;
class UChopItWeaponDefinition;

DECLARE_MULTICAST_DELEGATE_FourParams(FChopItAutoAttackPerformedNative, const FVector&, const FVector&, float, bool);

UCLASS(ClassGroup = (ChopIt), meta = (BlueprintSpawnableComponent))
class CHOPITCOMBAT_API UChopItAutoAttackComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItAutoAttackComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void PerformAttack();
	void SetWeaponDefinition(UChopItWeaponDefinition* NewDefinition);
	FChopItAutoAttackPerformedNative OnAttackPerformed;

private:
	void RestartAttackTimer();
	void HandleStatsChanged();

	UPROPERTY(EditAnywhere, Category = "ChopIt|Weapon")
	TObjectPtr<UChopItWeaponDefinition> WeaponDefinition;

	UPROPERTY(Transient)
	TObjectPtr<UChopItCombatStatsComponent> StatsComponent;

	FTimerHandle AttackTimerHandle;
};
