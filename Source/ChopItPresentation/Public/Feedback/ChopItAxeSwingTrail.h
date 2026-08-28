#pragma once

#include "GameFramework/Actor.h"
#include "ChopItAxeSwingTrail.generated.h"

class UStaticMeshComponent;

/** Brief, mesh-based slash cue for automatic axe attacks. */
UCLASS()
class CHOPITPRESENTATION_API AChopItAxeSwingTrail final : public AActor
{
	GENERATED_BODY()

public:
	AChopItAxeSwingTrail();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeTrail(const FVector& Forward, float Range, bool bHit);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> SlashMesh;
	float Age = 0.0f;
	FVector InitialScale = FVector::OneVector;
};
