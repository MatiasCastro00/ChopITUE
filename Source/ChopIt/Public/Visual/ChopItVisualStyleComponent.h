#pragma once

#include "Components/ActorComponent.h"
#include "Visual/ChopItVisualStylePreset.h"
#include "ChopItVisualStyleComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPostProcessComponent;
class UExponentialHeightFogComponent;
class APostProcessVolume;
class UChopItHealthComponent;
enum class EChopItCyclePhase : uint8;

/**
 * Global, Blueprint-callable controller for the ChopIt low-poly/retro look.
 * It runs in packaged builds, composites before Slate/UMG, and never replaces mesh materials.
 */
UCLASS(ClassGroup = (ChopIt), meta = (BlueprintSpawnableComponent))
class CHOPIT_API UChopItVisualStyleComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItVisualStyleComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "ChopIt|Visual Style")
	void ApplyPreset(EChopItVisualPreset NewPreset, float TransitionSeconds = 0.55f);

	UFUNCTION(BlueprintCallable, Category = "ChopIt|Visual Style")
	void SetGlobalIntensity(float NewIntensity, float TransitionSeconds = 0.35f);

	UFUNCTION(BlueprintCallable, Category = "ChopIt|Visual Style")
	void TriggerDamageDegradation(float Strength = 1.0f, float Duration = 0.45f);

	UFUNCTION(BlueprintCallable, Category = "ChopIt|Visual Style")
	void SetDangerDegradation(float NewIntensity, float TransitionSeconds = 0.75f);

	UFUNCTION(BlueprintCallable, Category = "ChopIt|Visual Style")
	void RestoreBasePreset(float TransitionSeconds = 0.55f);

	UFUNCTION(BlueprintPure, Category = "ChopIt|Visual Style")
	EChopItVisualPreset GetCurrentPreset() const { return BasePreset; }

	/** Opt-in helper for custom Blueprint actors that are not IChopItInteractable. */
	UFUNCTION(BlueprintCallable, Category = "ChopIt|Visual Style|Outline")
	static void SetActorOutline(AActor* Actor, bool bEnabled, int32 StencilValue = 3);

private:
	struct FRuntimeValues
	{
		float ColorSteps = 8.0f;
		float Contrast = 1.08f;
		float Saturation = 1.08f;
		float ExposureBias = 0.0f;
		float ColorGain = 1.0f;
		FLinearColor AmbientTint = FLinearColor::White;
		float AmbientTintStrength = 0.12f;
		float Pixelation = 1.0f;
		float VirtualResolutionHeight = 360.0f;
		float Dithering = 1.0f;
		float DitherStrength = 0.22f;
		float FilmGrain = 1.0f;
		float GrainIntensity = 0.045f;
		float Vignette = 1.0f;
		float VignetteIntensity = 0.22f;
		float ChromaticAberration = 1.0f;
		float ChromaticAberrationPixels = 0.35f;
		float Scanlines = 0.0f;
		float ScanlineIntensity = 0.0f;
		float ScreenDistortion = 0.0f;
		float DistortionIntensity = 0.0f;
		float DepthFog = 0.0f;
		float DepthFogDensity = 0.0f;
		float Outlines = 1.0f;
		float OutlineThickness = 1.15f;
		float GlobalOutline = 1.0f;
		FLinearColor GlobalOutlineColor = FLinearColor(0.01f, 0.018f, 0.014f, 1.0f);
		float GlobalOutlineThickness = 1.0f;
		float GlobalOutlineDepthThreshold = 0.018f;
		float GlobalOutlineNormalThreshold = 0.16f;
		float GlobalOutlineStrength = 0.92f;
		FLinearColor PlayerOutlineColor = FLinearColor(1.0f, 0.55f, 0.08f, 1.0f);
		FLinearColor EnemyOutlineColor = FLinearColor(1.0f, 0.14f, 0.06f, 1.0f);
		FLinearColor InteractableOutlineColor = FLinearColor(0.15f, 0.95f, 0.80f, 1.0f);
		FLinearColor FogColor = FLinearColor(0.13f, 0.34f, 0.32f, 1.0f);
		float FogDensity = 0.018f;
		float FogStartDistance = 180.0f;
		float FogMaxOpacity = 0.82f;
		float FogHeightFalloff = 0.22f;
		float BloomIntensity = 0.18f;
	};

	const UChopItVisualStylePreset* ResolvePreset(EChopItVisualPreset Preset) const;
	static FRuntimeValues ValuesFromPreset(const UChopItVisualStylePreset* Preset);
	static FRuntimeValues LerpValues(const FRuntimeValues& A, const FRuntimeValues& B, float Alpha);
	void ApplyCurrentValues();
	void ConfigurePostProcess();
	void RefreshOutlinedActors();
	void HandleActorSpawned(AActor* Actor);
	void ConfigureOutlineForActor(AActor* Actor);
	void BindPlayerDamage();
	void HandlePlayerDamaged(float Damage, bool bCritical, AActor* DamageSource, const FVector& ImpactLocation);

	UFUNCTION()
	void HandleCyclePhaseChanged(EChopItCyclePhase NewPhase, EChopItCyclePhase PreviousPhase, int32 Generation);

	UPROPERTY(EditAnywhere, Category = "ChopIt|Visual Style")
	EChopItVisualPreset DefaultPreset = EChopItVisualPreset::Hybrid;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Visual Style", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DefaultGlobalIntensity = 1.0f;

	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> PostProcessComponent;

	UPROPERTY(Transient)
	TObjectPtr<APostProcessVolume> PostProcessVolume;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PostProcessMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GlobalOutlineMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UExponentialHeightFogComponent> HeightFogComponent;

	TWeakObjectPtr<UChopItHealthComponent> BoundPlayerHealth;
	FDelegateHandle ActorSpawnedHandle;
	FRuntimeValues StartValues;
	FRuntimeValues TargetValues;
	FRuntimeValues CurrentValues;
	EChopItVisualPreset BasePreset = EChopItVisualPreset::Hybrid;
	float TransitionElapsed = 0.0f;
	float TransitionDuration = 0.0f;
	float CurrentGlobalIntensity = 1.0f;
	float StartGlobalIntensity = 1.0f;
	float TargetGlobalIntensity = 1.0f;
	float IntensityTransitionElapsed = 0.0f;
	float IntensityTransitionDuration = 0.0f;
	float DamagePulse = 0.0f;
	float DamagePulseDuration = 0.45f;
	float DangerIntensity = 0.0f;
	float TargetDangerIntensity = 0.0f;
	float OutlineRefreshElapsed = 0.0f;
	FString CaptureFilename;
	float CaptureElapsed = 0.0f;
	bool bCaptureRequested = false;
};
