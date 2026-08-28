#pragma once

#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "ChopItCharacter.generated.h"

class UCameraComponent;
class AChopItCabinHub;
class UChopItAutoAttackComponent;
class UChopItCombatStatsComponent;
class UChopItHealthComponent;
class UChopItInteractionComponent;
class UChopItWoodCargoComponent;
class UChopItWeaponLoadoutComponent;
class UInputAction;
class USpringArmComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UChopItHitFeedbackComponent;
class UChopItAttackFeedbackComponent;

/** Camera-relative top-down character used by the gameplay sandbox. */
UCLASS(Blueprintable)
class CHOPIT_API AChopItCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AChopItCharacter();
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	static FVector2D NormalizeMovementInput(const FVector2D& Input);

	UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UChopItInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }
	UChopItAutoAttackComponent* GetAutoAttackComponent() const { return AutoAttackComponent; }
	UChopItCombatStatsComponent* GetCombatStatsComponent() const { return CombatStatsComponent; }
	UChopItHealthComponent* GetHealthComponent() const { return HealthComponent; }
	UChopItWoodCargoComponent* GetWoodCargoComponent() const { return WoodCargoComponent; }
	UChopItWeaponLoadoutComponent* GetWeaponLoadoutComponent() const { return WeaponLoadoutComponent; }
	UTextRenderComponent* GetWoodCargoLabel() const { return WoodCargoLabel; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleInteract(const FInputActionValue& Value);

	UFUNCTION()
	void HandleWoodCargoChanged(int32 CurrentWood, int32 Capacity);

	UFUNCTION()
	void HandleQuotaChanged(int32 Progress, int32 Target, bool bComplete);

	UFUNCTION()
	void HandleBalanceChanged(int64 Balance, int64 Delta);

	UFUNCTION()
	void HandleCyclePhaseChanged(EChopItCyclePhase NewPhase, EChopItCyclePhase PreviousPhase, int32 Generation);

	UFUNCTION()
	void HandleCycleClockChanged(EChopItCyclePhase Phase, float RemainingSeconds);

	UFUNCTION()
	void HandleExperienceChanged(int32 Level, int32 CurrentExperience, int32 RequiredExperience, int32 PendingLevelUps);

	UFUNCTION()
	void HandleOffersChanged();
	void HandlePlayerDeath(AActor* DeadActor, AActor* DamageSource);

	void RefreshEconomyDebugLabel();
	void RefreshMovementStats();

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Visual")
	TObjectPtr<UStaticMeshComponent> BodyVisual;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Visual")
	TObjectPtr<UStaticMeshComponent> FacingMarker;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Interaction")
	TObjectPtr<UChopItInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Combat")
	TObjectPtr<UChopItCombatStatsComponent> CombatStatsComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Combat")
	TObjectPtr<UChopItHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Feedback")
	TObjectPtr<UChopItHitFeedbackComponent> HitFeedbackComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Feedback")
	TObjectPtr<UChopItAttackFeedbackComponent> AttackFeedbackComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Combat")
	TObjectPtr<UChopItAutoAttackComponent> AutoAttackComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Combat")
	TObjectPtr<UChopItWeaponLoadoutComponent> WeaponLoadoutComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Harvest")
	TObjectPtr<UChopItWoodCargoComponent> WoodCargoComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Harvest")
	TObjectPtr<UTextRenderComponent> WoodCargoLabel;

	UPROPERTY(Transient)
	TObjectPtr<AChopItCabinHub> CabinHub;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractAction;

	float BaseWalkSpeed = 650.0f;
};
