#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/ChopItInteractable.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Camera/ChopItCameraTypes.h"
#include "TimerManager.h"
#include "ChopItQuotaMachine.generated.h"

class UChopItChainDefinition;
class UChopItRopeComponent;
class UChopItTetherPathComponent;
class UChopItTetherReceiverComponent;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class AChopItCameraAnchor;
class UChopItCameraCue;

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
	FVector GetDeliveryIntakeWorldLocation() const;
	void NotifyWoodConsumed(int32 Units);
	UInstancedStaticMeshComponent* GetWoodChipPool() const { return WoodChipPool; }
	int32 GetWoodChipPoolSize() const { return WoodChipPoolSize; }
	int32 GetActiveWoodChipCount() const;
	bool IsDeathPresentationReady() const { return bDeathPresentationReady; }
	/** Called by the receiver after CharacterMovement, before Chaos. */
	void AdvancePlayerChain(float DeltaSeconds);
#if WITH_DEV_AUTOMATION_TESTS
	void BeginDeathSequenceForAutomation() { BeginDeathSequence(); }
	int32 GetDeathStallCountForAutomation() const { return DeathStallCount; }
	bool IsDeathSequenceActive() const { return bDeathSequenceActive; }
	float GetDeathUpdateMillisecondsForAutomation() const { return DeathUpdateMilliseconds; }
	int32 GetDeathCollisionQueriesForAutomation() const { return DeathCollisionQueries; }
	float GetDeathDesiredRopeLengthForAutomation() const { return DeathDesiredRopeLength; }
	float GetDeathRetractionSpeedForAutomation() const { return DeathCurrentRetractionSpeed; }
#endif

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
	void ApplyTetherConstraint(const FVector& CurrentEnd, const FVector& AcceptedEnd, float DeltaSeconds);
	void BeginDeathSequence();
	void UpdateDeathSequence(float DeltaSeconds);
	void ConsumeChainedPlayer();
	void UpdateDeathCrush(float DeltaSeconds);
	void BeginDefeatPresentation();
	void PlayDeathShake(float Scale);
	void UpdateChainVisuals();
	void UpdateReleasedChainLabel();
	void UpdateDeliveryReaction(float DeltaSeconds);
	void SpawnWoodChips(int32 Count);
	void UpdateWoodChips(float DeltaSeconds);
	void HideWoodChip(int32 PoolIndex);
	const UChopItChainDefinition* GetChainDefinition() const;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<UStaticMeshComponent> MachineVisual;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<UTextRenderComponent> QuotaLabel;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Cycle")
	TObjectPtr<UTextRenderComponent> LeverLabel;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Chain|07 Components")
	TObjectPtr<UTextRenderComponent> ReleasedChainLabel;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<UPointLightComponent> DeliveryGlow;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Quota")
	TObjectPtr<UInstancedStaticMeshComponent> WoodChipPool;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Quota|Juice", meta = (ClampMin = "32", ClampMax = "256"))
	int32 WoodChipPoolSize = 96;
	UPROPERTY(EditAnywhere, Category = "ChopIt|Quota|Juice", meta = (ClampMin = "1", ClampMax = "12"))
	int32 WoodChipsPerItem = 5;

	/** Read-only physical path adapter. */
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Chain|07 Components")
	TObjectPtr<UChopItTetherPathComponent> TetherPath;
	/** Authoritative chain and reel simulation. */
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

	struct FWoodChipParticle
	{
		bool bActive = false;
		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FRotator AngularVelocity = FRotator::ZeroRotator;
		FVector BaseScale = FVector(0.02f, 0.02f, 0.05f);
		float Age = 0.0f;
		float Lifetime = 0.8f;
	};

	TArray<FWoodChipParticle> WoodChips;
	FRandomStream DeliveryVisualRandom;
	FVector MachineBaseLocation = FVector::ZeroVector;
	FVector MachineBaseScale = FVector::OneVector;
	FRotator MachineBaseRotation = FRotator::ZeroRotator;
	float DeliveryReactionStrength = 0.0f;
	float DeliveryAnimationTime = 0.0f;
	float CurrentCableLength = 0.0f;
	int32 LastDisplayedReleasedDecimeters = INDEX_NONE;
	bool bHardLimited = false;
	bool bDeathSequenceActive = false;
	bool bChainedPlayerConsumed = false;
	bool bDeathCrushActive = false;
	bool bDeathPresentationReady = false;
	bool bDeathSlowMotionApplied = false;
	float DeathCrushElapsed = 0.0f;
	TArray<TWeakObjectPtr<AActor>> DeathBypassedActors;
	int32 DeathStallCount = 0;
	float DeathDesiredRopeLength = 0.0f;
	float DeathStartRopeLength = 0.0f;
	float CurrentDeathRetractionTime = 0.0f;
	float DeathCurrentRetractionSpeed = 0.0f;
	float DeathRetractionDebt = 0.0f;
	float DeathNoProgressTime = 0.0f;
	float DeathBestDistance = TNumericLimits<float>::Max();
	float DeathUpdateMilliseconds = 0.0f;
	int32 DeathCollisionQueries = 0;
	UPROPERTY(Transient)
	TObjectPtr<AChopItCameraAnchor> DeathCameraAnchor;
	UPROPERTY(Transient)
	TObjectPtr<UChopItCameraCue> DeathCameraCue;
	float DeliveryGlowRemaining = 0.0f;
	FTimerHandle ChainCreationTimer;
};
