#pragma once

#include "GameFramework/GameplayCameraComponent.h"
#include "Camera/ChopItCameraTypes.h"
#include "Core/CameraRigInstanceID.h"
#include "Core/CameraShakeInstanceID.h"
#include "ChopItCameraComponent.generated.h"

class AChopItCameraAnchor;
class UCameraShakeAsset;
class UChopItCameraCue;
class UChopItCameraEffectPreset;
class UMaterialInterface;
class UPrimitiveComponent;

USTRUCT()
struct FChopItOccludedPrimitiveState
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> Component;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;
};

UCLASS(ClassGroup=(ChopIt), meta=(BlueprintSpawnableComponent))
class CHOPITPRESENTATION_API UChopItCameraComponent final : public UGameplayCameraComponent
{
	GENERATED_BODY()
public:
	UChopItCameraComponent(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void PostLoad() override;
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	FChopItCameraHandle PushCue(
		const UChopItCameraCue* Cue,
		AChopItCameraAnchor* Anchor,
		AActor* Subject,
		float FieldOfViewOverride = -1.0f,
		float BlendInTimeOverride = -1.0f);
	void PopCue(FChopItCameraHandle Handle, bool bImmediate);
	FChopItCameraHandle PushEffect(const UChopItCameraEffectPreset* Preset, float DurationOverride);
	FChopItCameraHandle PlayShake(const UCameraShakeAsset* ShakeAsset, float Scale, const FVector& WorldOrigin);
	void StopRequest(FChopItCameraHandle Handle, bool bImmediate = false);
	FChopItCameraHandle PushInputLock(EChopItCameraInputLock Locks);
	void PopInputLock(FChopItCameraHandle Handle);
	void ResetGameplayCamera();

	void AddMouseLookInput(const FVector2D& Input, float DeltaSeconds);
	void AddGamepadLookInput(const FVector2D& Input, float DeltaSeconds);
	/** Backwards-compatible test/gameplay entry point; interpreted as gamepad-rate input. */
	void AddLookInput(const FVector2D& Input, float DeltaSeconds);
	void AddZoomInput(float Input);
	bool IsInputLocked(EChopItCameraInputLock Lock) const;
	EChopItCameraMode GetActiveMode() const { return ActiveMode; }
	FChopItGameplayCameraView GetGameplayView() const { return GameplayView; }
	bool ShouldOverridePlayerCameraManager() const
	{
		return ActiveMode != EChopItCameraMode::GameplayOrbit
			|| (PoseBlendDuration > KINDA_SMALL_NUMBER && PoseBlendElapsed < PoseBlendDuration);
	}

	static constexpr float MinPitch = -65.0f;
	// Nearly vertical: only avoids crossing the pole and flipping the orbit.
	static constexpr float MaxPitch = 89.0f;
	static constexpr float MinDistance = 550.0f;
	static constexpr float MaxDistance = 1400.0f;
	static constexpr float ZoomStep = 100.0f;

private:
	virtual UCameraAsset* OnCreateEvaluationContext() override;
	virtual void OnUpdateEvaluationContext(bool bForceApplyParameterOverrides) override;
	struct FCueRequest
	{
		TWeakObjectPtr<const UChopItCameraCue> Cue;
		TWeakObjectPtr<AChopItCameraAnchor> Anchor;
		TWeakObjectPtr<AActor> Subject;
		uint64 Sequence = 0;
		double ExpiresAt = 0.0;
		float FieldOfViewOverride = -1.0f;
		float BlendInTimeOverride = -1.0f;
		bool bRequiresAnchor = false;
		bool bRequiresSubject = false;
		FCameraRigInstanceID AssociatedRig;
	};
	struct FEffectRequest
	{
		FCameraRigInstanceID Rig;
		double ExpiresAt = 0.0;
	};
	struct FShakeRequest
	{
		FCameraShakeInstanceID Shake;
		float Scale = 1.0f;
	};

	void ResolveActiveCue();
	void UpdateGameplayTransform(float DeltaTime);
	void UpdateScriptedTransform();
	FVector ResolveSubjectFocus(const AActor* Subject) const;
	void ApplyRenderedCameraPose(float DeltaTime, const FTransform& TargetTransform, float TargetFieldOfView);
	void ConfigureWorldCameraCollision();
	void UpdateOcclusionTransparency();
	void RestoreOcclusionMaterials();
	void PruneInvalidRequests();
	FChopItCameraHandle NewHandle() const;

	TMap<FGuid, FCueRequest> CueRequests;
	TMap<FGuid, FEffectRequest> EffectRequests;
	TMap<FGuid, FShakeRequest> ShakeRequests;
	TMap<FGuid, EChopItCameraInputLock> ExternalInputLocks;
	TOptional<FChopItGameplayCameraView> SavedGameplayView;
	TWeakObjectPtr<AChopItCameraAnchor> ActiveAnchor;
	TWeakObjectPtr<AActor> ActiveSubject;
	TWeakObjectPtr<AChopItCameraAnchor> LastRenderedAnchor;
	TWeakObjectPtr<AActor> LastRenderedSubject;
	EChopItCameraMode ActiveMode = EChopItCameraMode::GameplayOrbit;
	EChopItCameraMode LastRenderedMode = EChopItCameraMode::GameplayOrbit;
	EChopItCameraInputLock ActiveLocks = EChopItCameraInputLock::None;
	FChopItGameplayCameraView GameplayView;
	float TargetDistance = 850.0f;
	float CurrentCollisionDistance = 850.0f;
	float ActiveFieldOfView = 85.0f;
	float ActiveBlendInTime = 0.25f;
	float LastCueBlendOutTime = 0.2f;
	float PoseBlendDuration = 0.0f;
	float PoseBlendElapsed = 0.0f;
	float BlendStartFieldOfView = 85.0f;
	float LastRenderedFieldOfView = 85.0f;
	FVector2D SmoothedMouseInput = FVector2D::ZeroVector;
	uint64 NextSequence = 1;
	uint64 ActiveCueSequence = 0;
	uint64 LastRenderedCueSequence = 0;
	FTransform BlendStartTransform = FTransform::Identity;
	FTransform LastRenderedTransform = FTransform::Identity;
	bool bHasRenderedCameraPose = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> OcclusionMaterial;

	UPROPERTY(Transient)
	TArray<FChopItOccludedPrimitiveState> OccludedPrimitives;

	/** Half-width of the sampled subject silhouette, in centimeters. */
	UPROPERTY(EditAnywhere, Category="ChopIt|Camera|Occlusion", meta=(ClampMin="1.0", UIMin="10.0", UIMax="200.0"))
	float OcclusionCorridorRadius = 60.0f;
};
