#pragma once

#include "GameFramework/Actor.h"
#include "ChopItDeliveryZone.generated.h"

class UChopItWoodCargoComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Pumps integral cargo batches into the daily quota while a player overlaps. */
UCLASS(Blueprintable)
class CHOPITWORLD_API AChopItDeliveryZone : public AActor
{
	GENERATED_BODY()

public:
	AChopItDeliveryZone();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	USphereComponent* GetDeliverySphere() const { return DeliverySphere; }

private:
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

	void TransferQuotaBatch();

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Delivery")
	TObjectPtr<USphereComponent> DeliverySphere;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Delivery")
	TObjectPtr<UStaticMeshComponent> ZoneVisual;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Delivery")
	TObjectPtr<UTextRenderComponent> ZoneLabel;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Delivery", meta = (ClampMin = "1"))
	int32 BatchSize = 1;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Delivery", meta = (ClampMin = "0.05"))
	float TransferInterval = 0.15f;

	TWeakObjectPtr<UChopItWoodCargoComponent> CandidateCargo;
	FTimerHandle TransferTimerHandle;
};
