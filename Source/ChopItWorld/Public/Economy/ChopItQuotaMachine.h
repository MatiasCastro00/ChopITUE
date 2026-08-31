#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/ChopItInteractable.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "TimerManager.h"
#include "ChopItQuotaMachine.generated.h"

class UChopItChainDefinition;
class UChopItRopeComponent;
class UChopItTetherPathComponent;
class UChopItTetherReceiverComponent;
class UInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Diegetic quota facade and owner of the single player tether reel. */
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
	void CreatePlayerChain(AActor* PlayerActor);
	void DestroyPlayerChain();
	void UpdateRetractableChain(float DeltaSeconds);
	void UpdateReel(float RouteLength, float DeltaSeconds);
	void UpdatePlayerTension(float RouteLength);
	void CorrectHardLimit(float RouteLength);
	void UpdateChainVisuals();
	bool SampleCableAtDistance(const TArray<FVector>& Points, float Distance, FVector& OutLocation, FVector& OutDirection) const;
	float GetFixedLinkLength() const;
	const UChopItChainDefinition* GetChainDefinition() const;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<UStaticMeshComponent> MachineVisual;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<UTextRenderComponent> QuotaLabel;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Cycle")
	TObjectPtr<UTextRenderComponent> LeverLabel;

	/** Authoritative collision route. */
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Chain|07 Components")
	TObjectPtr<UChopItTetherPathComponent> TetherPath;
	/** Per-span simulation used only to place the visual chain skin. */
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Chain|07 Components")
	TObjectPtr<UChopItRopeComponent> RopeSimulation;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Chain|07 Components")
	TObjectPtr<UInstancedStaticMeshComponent> ChainLinkVisuals;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UChopItChainDefinition> ChainDefinition;

	UPROPERTY(Transient)
	TObjectPtr<AActor> ChainedPlayer;
	UPROPERTY(Transient)
	TObjectPtr<UChopItTetherReceiverComponent> TetherReceiver;

	int32 DeployedChainLinkCount = 0;
	int32 TargetChainLinkCount = 0;
	float CurrentCableLength = 0.0f;
	float TargetCableLength = 0.0f;
	float CableReelVelocity = 0.0f;
	bool bHardLimited = false;
	FVector LastValidPlayerLocation = FVector::ZeroVector;
	bool bHasLastValidPlayerLocation = false;
	FTimerHandle ChainCreationTimer;
};
