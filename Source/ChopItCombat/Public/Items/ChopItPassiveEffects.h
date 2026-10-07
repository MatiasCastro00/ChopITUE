#pragma once
#include "Items/ChopItItemEffect.h"
#include "ChopItPassiveEffects.generated.h"

class UChopItHealthComponent;
class UChopItCombatStatsComponent;

UCLASS(DisplayName="Mandrake Stick")
class CHOPITCOMBAT_API UChopItMandrakeEffect : public UChopItItemEffect
{
	GENERATED_BODY()
public:
	UChopItMandrakeEffect();
	UPROPERTY(EditAnywhere, Category="Mandrake", meta=(ClampMin="0.1")) float Duration = 6.f;
	UPROPERTY(EditAnywhere, Category="Mandrake", meta=(ClampMin="1")) float Radius = 350.f;
	UPROPERTY(EditAnywhere, Category="Mandrake", meta=(ClampMin="0")) float DamagePerSecond = 8.f;
	virtual void HandleEvent_Implementation(const FChopItItemEventContext& Context) override;
};

UCLASS(DisplayName="Health Regeneration")
class CHOPITCOMBAT_API UChopItRegenerationEffect : public UChopItItemEffect
{
	GENERATED_BODY()
public:
	UChopItRegenerationEffect();
	/** BaseValue is health per second; interval changes frequency, not healing rate. */
	UPROPERTY(EditAnywhere, Category="Effect", meta=(ClampMin="0.05")) float Interval = 1.f;
	virtual void OnActivated_Implementation() override;
	virtual void OnDeactivated_Implementation() override;
private:
	void Regenerate();
	FTimerHandle Timer;
	TWeakObjectPtr<UChopItHealthComponent> Health;
};

UCLASS(DisplayName="Life Steal")
class CHOPITCOMBAT_API UChopItLifeStealEffect : public UChopItItemEffect
{
	GENERATED_BODY()
public:
	UChopItLifeStealEffect();
	virtual void OnActivated_Implementation() override;
	virtual void HandleEvent_Implementation(const FChopItItemEventContext& Context) override;
private:
	TWeakObjectPtr<UChopItHealthComponent> Health;
};

UCLASS(DisplayName="Wood Yield")
class CHOPITCOMBAT_API UChopItWoodYieldEffect : public UChopItItemEffect
{
	GENERATED_BODY()
public:
	UChopItWoodYieldEffect();
	virtual void OnActivated_Implementation() override;
	virtual void OnStacksChanged_Implementation() override;
	virtual void OnDeactivated_Implementation() override;
private:
	TWeakObjectPtr<UChopItCombatStatsComponent> Stats;
	FGuid ModifierHandle;
};

UCLASS(DisplayName="Infestation")
class CHOPITCOMBAT_API UChopItInfestationEffect : public UChopItItemEffect
{
	GENERATED_BODY()
public:
	UChopItInfestationEffect();
	virtual void HandleEvent_Implementation(const FChopItItemEventContext& Context) override;
	virtual void OnDeactivated_Implementation() override;
	/** Accepts an already calculated amount, also usable by transfer/synergy effects. */
	UFUNCTION(BlueprintCallable, Category="ChopIt|Items") void ApplyInfestation(UChopItHealthComponent* Target, float Amount);
	UFUNCTION(BlueprintPure, Category="ChopIt|Items") float GetInfestation(UChopItHealthComponent* Target) const;
private:
	void CheckExecution(UChopItHealthComponent* Target);
	void Notify(UChopItHealthComponent* Target, float Amount, EChopItItemEvent Event);
	struct FInfestationState { float Amount = 0.f; FDelegateHandle HealthChanged; };
	TMap<TWeakObjectPtr<UChopItHealthComponent>, FInfestationState> Targets;
};
