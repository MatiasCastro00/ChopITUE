#pragma once

#include "Components/ActorComponent.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "ChopItWorldPresentationComponent.generated.h"

class UAudioComponent;
class USoundBase;
class AChopItFireflySwarm;

/** Provisional observer that maps cycle state to world lighting without owning rules. */
UCLASS(ClassGroup = (ChopIt), meta = (BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItWorldPresentationComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItWorldPresentationComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandlePhaseChanged(EChopItCyclePhase NewPhase, EChopItCyclePhase PreviousPhase, int32 Generation);
	void TransitionMusic(EChopItCyclePhase NewPhase);
	USoundBase* ResolveMusic(EChopItCyclePhase Phase) const;
	void UpdateFireflies(EChopItCyclePhase NewPhase);

	UPROPERTY(EditAnywhere, Category = "ChopIt|Cycle|Presentation", meta = (ClampMin = "0.1"))
	float LightTransitionDuration = 2.5f;

	UPROPERTY(EditAnywhere, Category="ChopIt|Cycle|Audio")
	TSoftObjectPtr<USoundBase> DayMusic;

	UPROPERTY(EditAnywhere, Category="ChopIt|Cycle|Audio")
	TSoftObjectPtr<USoundBase> DuskMusic;

	UPROPERTY(EditAnywhere, Category="ChopIt|Cycle|Audio")
	TSoftObjectPtr<USoundBase> NightMusic;

	UPROPERTY(EditAnywhere, Category="ChopIt|Cycle|Audio")
	TSoftObjectPtr<USoundBase> EliteMusic;

	UPROPERTY(EditAnywhere, Category="ChopIt|Cycle|Audio", meta=(ClampMin="0.0", ClampMax="5.0"))
	float MusicCrossfadeDuration = 1.5f;

	UPROPERTY(EditAnywhere, Category="ChopIt|Cycle|Night", meta=(ClampMin="1", ClampMax="128"))
	int32 FireflyCount = 42;

	UPROPERTY(EditAnywhere, Category="ChopIt|Cycle|Night", meta=(ClampMin="100.0"))
	float FireflyHorizontalRadius = 1900.0f;

	UPROPERTY(EditAnywhere, Category="ChopIt|Cycle|Night", meta=(ClampMin="100.0"))
	float FireflyMaximumHeight = 380.0f;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicA;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicB;

	UPROPERTY(Transient)
	TObjectPtr<AChopItFireflySwarm> FireflySwarm;

	TWeakObjectPtr<class UDirectionalLightComponent> DirectionalLight;
	FLinearColor TransitionStartColor = FLinearColor::White;
	FLinearColor TransitionTargetColor = FLinearColor::White;
	float TransitionStartIntensity = 5.0f;
	float TransitionTargetIntensity = 5.0f;
	float TransitionElapsed = 0.0f;
	bool bMusicAActive = false;
};
