#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/ChopItInteractable.h"
#include "Camera/ChopItCameraTypes.h"
#include "ChopItCameraDemoTrigger.generated.h"

class AChopItCameraAnchor;
class UStaticMeshComponent;

/** Sandbox-only interaction that exercises cue stacking, PP, shake and restoration. */
UCLASS()
class CHOPITPRESENTATION_API AChopItCameraDemoTrigger final : public AActor, public IChopItInteractable
{
	GENERATED_BODY()
public:
	AChopItCameraDemoTrigger();
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	class UChopItCameraDirectorSubsystem* GetDirector() const;
	void PlayShakeStep();
	void RemoveEffectStep();
	void RestoreStep();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
	UPROPERTY(Transient) TObjectPtr<AChopItCameraAnchor> RuntimeAnchor;
	FChopItCameraHandle CueHandle;
	FChopItCameraHandle EffectHandle;
	FTimerHandle ShakeTimer;
	FTimerHandle EffectTimer;
	FTimerHandle RestoreTimer;
};
