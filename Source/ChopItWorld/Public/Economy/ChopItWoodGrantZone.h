#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/ChopItInteractable.h"
#include "ChopItWoodGrantZone.generated.h"

class UChopItWoodCargoComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Clearly marked development helper that tops cargo up to a stress-test amount. */
UCLASS(Blueprintable)
class CHOPITWORLD_API AChopItWoodGrantZone : public AActor, public IChopItInteractable
{
	GENERATED_BODY()

public:
	AChopItWoodGrantZone();
	virtual void BeginPlay() override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool Interact_Implementation(AActor* Interactor) override;
	USphereComponent* GetGrantSphere() const { return GrantSphere; }
	int32 GetTargetWood() const { return TargetWood; }

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

	void RefreshPrompt();

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Testing")
	TObjectPtr<USphereComponent> GrantSphere;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Testing")
	TObjectPtr<UStaticMeshComponent> ZoneVisual;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Testing")
	TObjectPtr<UTextRenderComponent> ZoneLabel;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Testing", meta = (ClampMin = "1"))
	int32 TargetWood = 200;

	TWeakObjectPtr<UChopItWoodCargoComponent> NearbyCargo;
};
