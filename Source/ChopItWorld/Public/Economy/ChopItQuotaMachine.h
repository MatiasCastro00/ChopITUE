#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/ChopItInteractable.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "TimerManager.h"
#include "ChopItQuotaMachine.generated.h"

class UChopItQuotaComponent;
class UChopItChainDefinition;
class UCableComponent;
class UInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Diegetic quota facade; it observes quota state but never mutates cargo. */
UCLASS(Blueprintable)
class CHOPITWORLD_API AChopItQuotaMachine : public AActor, public IChopItInteractable
{
	GENERATED_BODY()

public:
	AChopItQuotaMachine();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool Interact_Implementation(AActor* Interactor) override;
	void SetChainDefinition(UChopItChainDefinition* InChainDefinition) { ChainDefinition = InChainDefinition; }

private:
	UFUNCTION()
	void HandleQuotaChanged(int32 Progress, int32 Target, bool bComplete);

	UFUNCTION()
	void HandlePhaseChanged(EChopItCyclePhase NewPhase, EChopItCyclePhase PreviousPhase, int32 Generation);

	UFUNCTION()
	void HandleClockChanged(EChopItCyclePhase Phase, float RemainingSeconds);

	void RefreshLeverLabel();
	void TryCreatePlayerChain();
	void UpdateRetractableChain(float DeltaSeconds);
	void UpdateReel(float HorizontalDistance, float DeltaSeconds);
	void EnforceCableTension(AActor* PlayerActor);
	void EnforceMaximumCableLength(AActor* PlayerActor);
	void UpdateChainVisuals();
	int32 CalculateRequiredLinkCount(float HorizontalDistance) const;
	void CreatePlayerChain(AActor* PlayerActor, int32 DesiredLinkCount);
	bool SampleCableAtDistance(
		const TArray<FVector>& Points,
		float Distance,
		FVector& OutLocation,
		FVector& OutDirection) const;
	float GetFixedLinkLength() const;
	const UChopItChainDefinition* GetChainDefinition() const;
	void DestroyPlayerChain();

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<UStaticMeshComponent> MachineVisual;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<UTextRenderComponent> QuotaLabel;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Cycle")
	TObjectPtr<UTextRenderComponent> LeverLabel;

	/** Verlet simulation used for gravity, wrapping and world collision. */
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Chain|07 Components")
	TObjectPtr<UCableComponent> ChainCable;

	/** Collisionless visual links sampled along the simulated cable. */
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Chain|07 Components")
	TObjectPtr<UInstancedStaticMeshComponent> ChainLinkVisuals;

	/** All chain behavior and appearance comes from this shared Data Asset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UChopItChainDefinition> ChainDefinition;

	UPROPERTY(Transient)
	TObjectPtr<AActor> ChainedPlayer;

	int32 DeployedChainLinkCount = 0;
	int32 TargetChainLinkCount = 0;
	float CurrentCableLength = 0.0f;
	float TargetCableLength = 0.0f;
	float CableReelVelocity = 0.0f;
	int32 TensionMinimumLinkCount = 0;
	float TensionReleaseReferenceDistance = 0.0f;
	bool bWasChainAtLimit = false;
	FVector LastAcceptedPlayerLocation = FVector::ZeroVector;
	bool bHasLastAcceptedPlayerLocation = false;

	FTimerHandle ChainCreationTimer;
};
