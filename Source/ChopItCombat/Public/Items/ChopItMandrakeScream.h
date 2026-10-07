#pragma once

#include "GameFramework/Actor.h"
#include "ChopItMandrakeScream.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UAudioComponent;

/** Temporary, stationary mandrake. Its pulses use the normal combat damage path. */
UCLASS()
class CHOPITCOMBAT_API AChopItMandrakeScream : public AActor
{
	GENERATED_BODY()
public:
	AChopItMandrakeScream();
	virtual void Tick(float DeltaSeconds) override;
	void Initialize(AActor* InOwner, float InRadius, float InDamagePerSecond, float InDuration);
private:
	void DamagePulse();
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Leaves;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftEye;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> RightEye;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mouth;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SoundRings;
	UPROPERTY() TObjectPtr<UPointLightComponent> Glow;
	UPROPERTY() TObjectPtr<UAudioComponent> Scream;
	TWeakObjectPtr<AActor> DamageOwner;
	FTimerHandle DamageTimer;
	float Radius = 350.f;
	float DamagePerSecond = 8.f;
	float Age = 0.f;
};
