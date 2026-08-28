#pragma once

#include "GameFramework/Actor.h"
#include "ChopItFeedbackBurst.generated.h"

class UInstancedStaticMeshComponent;

/** Short-lived physical-looking fragments for wood and foliage impacts. */
UCLASS()
class CHOPITPRESENTATION_API AChopItFeedbackBurst final : public AActor
{
	GENERATED_BODY()

public:
	AChopItFeedbackBurst();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeBurst(const FVector& AwayFromImpact, bool bCritical, bool bDeathBurst, float Density);

private:
	struct FFragmentState
	{
		int32 InstanceIndex = INDEX_NONE;
		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector Scale = FVector::OneVector;
	};

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> BarkFragments;

	TArray<FFragmentState> Fragments;
	float Age = 0.0f;
};
