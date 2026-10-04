#pragma once

#include "GameFramework/Actor.h"
#include "ChopItAxeSwingTrail.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;

/** One playback of the authored axe slash for each automatic attack. */
UCLASS()
class CHOPITPRESENTATION_API AChopItAxeSwingTrail final : public AActor
{
	GENERATED_BODY()

public:
	AChopItAxeSwingTrail();
	void InitializeTrail(const FVector& Forward, float Range, bool bHit, float EffectsDensity);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNiagaraComponent> SlashParticles;

	UPROPERTY(EditDefaultsOnly, Category = "ChopIt|VFX")
	TObjectPtr<UNiagaraSystem> SlashSystem;
};
