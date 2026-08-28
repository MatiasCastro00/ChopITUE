#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChopItDamageNumber.generated.h"

class UTextRenderComponent;

/** A short-lived, camera-facing damage number with an elastic entrance and white fade-out. */
UCLASS(NotPlaceable)
class CHOPITPRESENTATION_API AChopItDamageNumber final : public AActor
{
	GENERATED_BODY()

public:
	AChopItDamageNumber();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeDamageNumber(float Damage, bool bCritical);

private:
	UPROPERTY(VisibleAnywhere, Category="ChopIt|Feedback")
	TObjectPtr<UTextRenderComponent> TextRender;

	FVector SpawnLocation = FVector::ZeroVector;
	FVector Drift = FVector::ZeroVector;
	FLinearColor StartColor = FLinearColor::White;
	float Age = 0.0f;
	float BaseScale = 1.0f;
	float RollOffset = 0.0f;
	bool bInitialized = false;

	static constexpr float PopDuration = 0.22f;
	static constexpr float Lifetime = 0.82f;
};
