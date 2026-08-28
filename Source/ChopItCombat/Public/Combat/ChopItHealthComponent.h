#pragma once

#include "Combat/ChopItDamageTypes.h"
#include "Components/ActorComponent.h"
#include "ChopItHealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FChopItHealthChangedNative, float, float, AActor*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FChopItDeathNative, AActor*, AActor*);
DECLARE_MULTICAST_DELEGATE_FourParams(FChopItDamageReceivedNative, float, bool, AActor*, const FVector&);

UCLASS(ClassGroup = (ChopIt), meta = (BlueprintSpawnableComponent))
class CHOPITCOMBAT_API UChopItHealthComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItHealthComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	float ApplyDamage(const FChopItDamageSpec& DamageSpec, AActor* DamageSource, const FVector& ImpactLocation = FVector::ZeroVector);
	void ResetHealth();
	void SetMaxHealth(float NewMaxHealth);
	bool IsAlive() const { return CurrentHealth > 0.0f; }
	float GetCurrentHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }

	FChopItHealthChangedNative OnHealthChanged;
	FChopItDeathNative OnDeath;
	FChopItDamageReceivedNative OnDamageReceived;

private:
	UPROPERTY(EditAnywhere, Category = "ChopIt|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Health")
	float CurrentHealth = 100.0f;

	bool bDeathBroadcast = false;
};
