#pragma once

#include "Components/ActorComponent.h"
#include "ChopItAttackFeedbackComponent.generated.h"

/** Spawns presentation-only axe trails from the auto-attack event. */
UCLASS(ClassGroup=(ChopIt), meta=(BlueprintSpawnableComponent))
class CHOPITPRESENTATION_API UChopItAttackFeedbackComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItAttackFeedbackComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleAttackPerformed(const FVector& Origin, const FVector& Forward, float Range, bool bHit);
};
