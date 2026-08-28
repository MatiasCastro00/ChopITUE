#pragma once

#include "GameFramework/Actor.h"
#include "ChopItSellZone.generated.h"

class UChopItQuotaComponent;
class UChopItEconomyComponent;
class UChopItWoodCargoComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Converts surplus wood into money only after quota completion. */
UCLASS(Blueprintable)
class CHOPITWORLD_API AChopItSellZone : public AActor
{
	GENERATED_BODY()

public:
	AChopItSellZone();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	USphereComponent* GetSellSphere() const { return SellSphere; }
	static bool CanSell(
		const UChopItQuotaComponent* Quota,
		const UChopItWoodCargoComponent* Cargo,
		const UChopItEconomyComponent* Economy);

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

	UFUNCTION()
	void HandleQuotaChanged(int32 Progress, int32 Target, bool bComplete);

	void SellBatch();

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Sell")
	TObjectPtr<USphereComponent> SellSphere;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Sell")
	TObjectPtr<UStaticMeshComponent> ZoneVisual;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Sell")
	TObjectPtr<UTextRenderComponent> ZoneLabel;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Sell", meta = (ClampMin = "1"))
	int32 BatchSize = 1;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Sell", meta = (ClampMin = "1"))
	int64 MoneyPerWood = 4;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Sell", meta = (ClampMin = "0.05"))
	float TransferInterval = 0.15f;

	TWeakObjectPtr<UChopItWoodCargoComponent> CandidateCargo;
	FTimerHandle TransferTimerHandle;
};
