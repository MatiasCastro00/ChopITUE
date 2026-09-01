#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/ChopItInteractable.h"
#include "ChopItDeliveryZone.generated.h"

class AChopItQuotaMachine;
class UChopItQuotaComponent;
class UChopItWoodCargoComponent;
class UInstancedStaticMeshComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Interactive quota intake with pooled, parabolic log visuals. */
UCLASS(Blueprintable)
class CHOPITWORLD_API AChopItDeliveryZone : public AActor, public IChopItInteractable
{
	GENERATED_BODY()

public:
	AChopItDeliveryZone();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool Interact_Implementation(AActor* Interactor) override;

	USphereComponent* GetDeliverySphere() const { return DeliverySphere; }
	UInstancedStaticMeshComponent* GetLogPool() const { return LogPool; }
	UInstancedStaticMeshComponent* GetTrailPool() const { return TrailPool; }
	int32 GetPoolSize() const { return PoolSize; }
	int32 GetTrailSegmentsPerLog() const { return TrailSegmentsPerLog; }
	int32 GetActiveFlightCount() const;
	void SetTargetMachine(AChopItQuotaMachine* InMachine) { TargetMachine = InMachine; }

	static FVector EvaluateParabolicFlight(
		const FVector& Start,
		const FVector& Target,
		const FVector& LateralOffset,
		float ArcHeight,
		float Alpha);

private:
	struct FDeliveryFlight
	{
		bool bActive = false;
		FVector Start = FVector::ZeroVector;
		FVector Target = FVector::ZeroVector;
		FVector LateralOffset = FVector::ZeroVector;
		FRotator InitialRotation = FRotator::ZeroRotator;
		FRotator RotationRate = FRotator::ZeroRotator;
		float Elapsed = 0.0f;
		float Duration = 0.7f;
		float ArcHeight = 300.0f;
	};

	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	void LaunchNextLog();
	void CompleteFlight(int32 PoolIndex);
	void StopLaunching();
	void RefreshPrompt();
	void RefreshTickState();
	void HidePoolInstance(int32 PoolIndex);
	void HideTrailInstances(int32 PoolIndex);
	void UpdateTrailInstances(int32 PoolIndex, float FlightAlpha);
	UChopItQuotaComponent* ResolveQuota() const;
	AChopItQuotaMachine* ResolveTargetMachine();

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Delivery")
	TObjectPtr<USphereComponent> DeliverySphere;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Delivery")
	TObjectPtr<UStaticMeshComponent> ZoneVisual;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Delivery")
	TObjectPtr<UTextRenderComponent> ZoneLabel;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Delivery")
	TObjectPtr<UInstancedStaticMeshComponent> LogPool;

	/** Small pooled glow/dust beads sampled behind each parabolic log. */
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Delivery")
	TObjectPtr<UInstancedStaticMeshComponent> TrailPool;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Delivery", meta = (ClampMin = "8", ClampMax = "128"))
	int32 PoolSize = 32;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Delivery", meta = (ClampMin = "3", ClampMax = "12"))
	int32 TrailSegmentsPerLog = 7;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Delivery", meta = (ClampMin = "0.02"))
	float LaunchInterval = 0.045f;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Delivery", meta = (ClampMin = "0.1"))
	float MinimumFlightDuration = 0.55f;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Delivery", meta = (ClampMin = "0.1"))
	float MaximumFlightDuration = 0.82f;

	TWeakObjectPtr<UChopItWoodCargoComponent> NearbyCargo;
	TWeakObjectPtr<UChopItWoodCargoComponent> ActiveCargo;
	TWeakObjectPtr<AChopItQuotaMachine> TargetMachine;
	TArray<FDeliveryFlight> Flights;
	FRandomStream VisualRandom;
	FTimerHandle LaunchTimerHandle;
	int32 PendingUnits = 0;
	float VisualTime = 0.0f;
};
