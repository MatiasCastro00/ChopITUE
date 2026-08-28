#pragma once

#include "GameFramework/Actor.h"
#include "ChopItLeafFall.generated.h"

class UInstancedStaticMeshComponent;

/** Releases leaves gradually from a tree canopy and carries them down on a wind drift. */
UCLASS()
class CHOPITPRESENTATION_API AChopItLeafFall final : public AActor
{
	GENERATED_BODY()

public:
	AChopItLeafFall();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeLeafFall(float Density, bool bHeavyFall);

private:
	struct FLeafState
	{
		int32 InstanceIndex = INDEX_NONE;
		FVector Location = FVector::ZeroVector;
		FVector DriftDirection = FVector::ForwardVector;
		float Phase = 0.0f;
		float FallSpeed = 80.0f;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector Scale = FVector::OneVector;
	};

	void SpawnLeaf();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> Leaves;

	TArray<FLeafState> ActiveLeaves;
	int32 RemainingLeaves = 0;
	float SpawnAccumulator = 0.0f;
	float Age = 0.0f;
	bool bHeavy = false;
};
