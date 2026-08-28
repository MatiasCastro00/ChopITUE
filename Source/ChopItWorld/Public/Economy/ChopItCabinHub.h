#pragma once

#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "GameFramework/Actor.h"
#include "ChopItCabinHub.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Spatial anchor and visual composition for the economy hub. */
UCLASS(Blueprintable)
class CHOPITWORLD_API AChopItCabinHub : public AActor
{
	GENERATED_BODY()

public:
	AChopItCabinHub();
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandlePhaseChanged(EChopItCyclePhase NewPhase, EChopItCyclePhase PreviousPhase, int32 Generation);

	void SetGuidanceActive(bool bActive);

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Hub")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Hub")
	TObjectPtr<UStaticMeshComponent> CabinVisual;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Hub")
	TObjectPtr<UPointLightComponent> GuidanceLight;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Hub")
	TObjectPtr<UTextRenderComponent> GuidanceLabel;
};
