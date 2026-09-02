#pragma once

#include "Engine/DataAsset.h"
#include "ChopItVisualStylePreset.generated.h"

UENUM(BlueprintType)
enum class EChopItVisualPreset : uint8
{
	Megabonk,
	MachineParty,
	Hybrid
};

/** Build-safe visual-style values consumed by UChopItVisualStyleComponent. */
UCLASS(BlueprintType)
class CHOPIT_API UChopItVisualStylePreset final : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	EChopItVisualPreset Preset = EChopItVisualPreset::Hybrid;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color", meta = (ClampMin = "2.0", ClampMax = "32.0"))
	float ColorSteps = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float Contrast = 1.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float Saturation = 1.08f;

	/** Manual camera exposure used by the unbound post process. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color", meta = (ClampMin = "-3.0", ClampMax = "3.0"))
	float ExposureBias = 0.0f;

	/** Display-referred gain applied before palette quantization. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ColorGain = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor AmbientTint = FLinearColor(0.88f, 0.91f, 0.84f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AmbientTintStrength = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixelation")
	bool bPixelation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pixelation", meta = (ClampMin = "144.0", ClampMax = "1080.0"))
	float VirtualResolutionHeight = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dithering")
	bool bDithering = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dithering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DitherStrength = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grain")
	bool bFilmGrain = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grain", meta = (ClampMin = "0.0", ClampMax = "0.25"))
	float GrainIntensity = 0.045f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lens")
	bool bVignette = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lens", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VignetteIntensity = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lens")
	bool bChromaticAberration = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lens", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float ChromaticAberrationPixels = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Optional Effects")
	bool bScanlines = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Optional Effects", meta = (ClampMin = "0.0", ClampMax = "0.25"))
	float ScanlineIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Optional Effects")
	bool bScreenDistortion = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Optional Effects", meta = (ClampMin = "0.0", ClampMax = "0.05"))
	float DistortionIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Optional Effects")
	bool bDepthFog = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Optional Effects", meta = (ClampMin = "0.0", ClampMax = "0.001"))
	float DepthFogDensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline")
	bool bOutlines = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline", meta = (ClampMin = "0.5", ClampMax = "4.0"))
	float OutlineThickness = 1.15f;

	/** Screen-space depth/normal outline shared by all opaque and masked geometry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline|Global")
	bool bGlobalOutline = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline|Global")
	FLinearColor GlobalOutlineColor = FLinearColor(0.01f, 0.018f, 0.014f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline|Global", meta = (ClampMin = "0.5", ClampMax = "3.0"))
	float GlobalOutlineThickness = 1.0f;

	/** Relative scene-depth discontinuity needed to produce an edge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline|Global", meta = (ClampMin = "0.001", ClampMax = "0.2"))
	float GlobalOutlineDepthThreshold = 0.018f;

	/** One minus the neighbouring normal dot product needed to produce an edge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline|Global", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float GlobalOutlineNormalThreshold = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline|Global", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GlobalOutlineStrength = 0.92f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline")
	FLinearColor PlayerOutlineColor = FLinearColor(1.0f, 0.55f, 0.08f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline")
	FLinearColor EnemyOutlineColor = FLinearColor(1.0f, 0.14f, 0.06f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline")
	FLinearColor InteractableOutlineColor = FLinearColor(0.15f, 0.95f, 0.80f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FLinearColor FogColor = FLinearColor(0.13f, 0.34f, 0.32f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere", meta = (ClampMin = "0.0", ClampMax = "0.08"))
	float FogDensity = 0.018f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere", meta = (ClampMin = "0.0", ClampMax = "10000.0"))
	float FogStartDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FogMaxOpacity = 0.82f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere", meta = (ClampMin = "0.001", ClampMax = "2.0"))
	float FogHeightFalloff = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float BloomIntensity = 0.18f;
};
