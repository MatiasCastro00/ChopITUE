#pragma once

#include "Components/ActorComponent.h"
#include "ChopItHitFeedbackComponent.generated.h"

class UPrimitiveComponent;
class USpringArmComponent;

/** Presentation-only response to health events. Never changes combat state. */
UCLASS(ClassGroup=(ChopIt), meta=(BlueprintSpawnableComponent))
class CHOPITPRESENTATION_API UChopItHitFeedbackComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItHitFeedbackComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void SetVisualComponent(UPrimitiveComponent* NewVisual);
	void SetWoodenTarget(bool bNewWoodenTarget) { bWoodenTarget = bNewWoodenTarget; }
	void SetFoliageComponent(UPrimitiveComponent* NewFoliage) { FoliageComponent = NewFoliage; }

private:
	void HandleDamageReceived(float Damage, bool bCritical, AActor* DamageSource, const FVector& ImpactLocation);
	void HandleDeath(AActor* DeadActor, AActor* DamageSource);
	void RestorePulse();
	void RestoreCamera();

	UPROPERTY(EditAnywhere, Category="ChopIt|Feedback", meta=(ClampMin="0.02", ClampMax="0.5"))
	float PulseDuration = 0.09f;

	bool bWoodenTarget = false;

	TWeakObjectPtr<UPrimitiveComponent> VisualComponent;
	TWeakObjectPtr<UPrimitiveComponent> FoliageComponent;
	TWeakObjectPtr<USpringArmComponent> CameraBoom;
	FVector OriginalVisualScale = FVector::OneVector;
	FVector OriginalCameraOffset = FVector::ZeroVector;
	FTimerHandle PulseTimerHandle;
	FTimerHandle CameraTimerHandle;
};
