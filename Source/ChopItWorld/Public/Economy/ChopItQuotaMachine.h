#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/ChopItInteractable.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "ChopItQuotaMachine.generated.h"

class UChopItQuotaComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Diegetic quota facade; it observes quota state but never mutates cargo. */
UCLASS(Blueprintable)
class CHOPITWORLD_API AChopItQuotaMachine : public AActor, public IChopItInteractable
{
	GENERATED_BODY()

public:
	AChopItQuotaMachine();
	virtual void BeginPlay() override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool Interact_Implementation(AActor* Interactor) override;

private:
	UFUNCTION()
	void HandleQuotaChanged(int32 Progress, int32 Target, bool bComplete);

	UFUNCTION()
	void HandlePhaseChanged(EChopItCyclePhase NewPhase, EChopItCyclePhase PreviousPhase, int32 Generation);

	UFUNCTION()
	void HandleClockChanged(EChopItCyclePhase Phase, float RemainingSeconds);

	void RefreshLeverLabel();

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<UStaticMeshComponent> MachineVisual;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<UTextRenderComponent> QuotaLabel;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Cycle")
	TObjectPtr<UTextRenderComponent> LeverLabel;
};
