#pragma once
#include "Components/ActorComponent.h"
#include "ChopItScreamDebuffComponent.generated.h"

class UCharacterMovementComponent;
class UChopItHealthComponent;
class UNiagaraComponent;
class UNiagaraSystem;

/** Refreshing sonic slow: overlapping mandrakes never multiply the movement penalty. */
UCLASS()
class CHOPITCOMBAT_API UChopItScreamDebuffComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UChopItScreamDebuffComponent();
	void Refresh(float Multiplier, float Duration);
	bool IsSlowed() const { return bActive; }
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	void ClearSlow();
	void HandleDeath(AActor*, AActor*);
	UPROPERTY() TObjectPtr<UNiagaraSystem> ConfusionSystem;
	UPROPERTY() TObjectPtr<UNiagaraComponent> Confusion;
	TWeakObjectPtr<UCharacterMovementComponent> Movement;
	TWeakObjectPtr<UChopItHealthComponent> Health;
	FTimerHandle Expiry;
	float OriginalSpeed = 0.f;
	float AppliedSpeed = 0.f;
	float AppliedMultiplier = 1.f;
	float Orbit = 0.f;
	bool bActive = false;
};
