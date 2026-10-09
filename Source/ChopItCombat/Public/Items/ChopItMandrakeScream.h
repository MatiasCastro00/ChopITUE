#pragma once

#include "GameFramework/Actor.h"
#include "ChopItMandrakeScream.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;
class UPointLightComponent;
class UAudioComponent;

/** Temporary, stationary mandrake. Its pulses use the normal combat damage path. */
UCLASS()
class CHOPITCOMBAT_API AChopItMandrakeScream : public AActor
{
	GENERATED_BODY()
public:
	AChopItMandrakeScream();
	UFUNCTION(BlueprintCallable, Category="Mandrake")
	void StartVisualPreview(float PreviewDuration = 3600.f);
	virtual void Tick(float DeltaSeconds) override;
	void Initialize(AActor* InOwner, float InRadius, float InDamagePerSecond, float InDuration, float InSlowMultiplier = .65f, float InSlowDuration = 1.f);
private:
	void DamagePulse();
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY() TObjectPtr<UNiagaraComponent> SoundWaves;
	UPROPERTY() TObjectPtr<UNiagaraComponent> LeftSplash;
	UPROPERTY() TObjectPtr<UNiagaraComponent> RightSplash;
	UPROPERTY() TObjectPtr<UNiagaraComponent> Emergence;
	UPROPERTY() TObjectPtr<UPointLightComponent> Glow;
	UPROPERTY() TObjectPtr<UAudioComponent> Scream;
	TWeakObjectPtr<AActor> DamageOwner;
	FTimerHandle DamageTimer;
	float Radius = 350.f;
	float DamagePerSecond = 8.f;
	float Age = 0.f;
	float ModelScale = 100.f;
	float Lifetime = 6.f;
	float SlowMultiplier = .65f;
	float SlowDuration = 1.f;
};
