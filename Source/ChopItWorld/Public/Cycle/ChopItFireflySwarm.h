#pragma once

#include "GameFramework/Actor.h"
#include "ChopItFireflySwarm.generated.h"

class UInstancedStaticMeshComponent;

/** Lightweight ambient fireflies. One instanced mesh draw call is used for the whole swarm. */
UCLASS(NotBlueprintable)
class CHOPITWORLD_API AChopItFireflySwarm final : public AActor
{
	GENERATED_BODY()

public:
	AChopItFireflySwarm();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void Configure(int32 InCount, float InHorizontalRadius, float InMaximumHeight);

private:
	struct FFireflyState
	{
		FVector Position = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Phase = 0.0f;
		float FlightRate = 1.0f;
		float PulseRate = 1.0f;
	};

	void PopulateSwarm();

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Night")
	TObjectPtr<UInstancedStaticMeshComponent> FireflyInstances;

	int32 FireflyCount = 42;
	float HorizontalRadius = 1900.0f;
	float MaximumHeight = 380.0f;
	TArray<FFireflyState> Fireflies;
};
