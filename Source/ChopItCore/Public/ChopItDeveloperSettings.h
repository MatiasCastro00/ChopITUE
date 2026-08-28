#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "ChopItDeveloperSettings.generated.h"

/** Bootstrap references and diagnostics that are safe to configure without code changes. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="ChopIt"))
class CHOPITCORE_API UChopItDeveloperSettings final : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	/** Map loaded by packaged builds until the frontend exists. */
	UPROPERTY(Config, EditAnywhere, Category="Bootstrap", meta=(AllowedClasses="/Script/Engine.World"))
	FSoftObjectPath StartupMap;

	/** Enables additional startup diagnostics without recompiling. */
	UPROPERTY(Config, EditAnywhere, Category="Diagnostics")
	bool bEnableVerboseStartupLogs = false;

	/** Brief scale/color-independent pulse when an actor receives damage. */
	UPROPERTY(Config, EditAnywhere, Category="Accessibility|Feedback")
	bool bEnableHitFlash = true;

	/** Optional world-space damage values. */
	UPROPERTY(Config, EditAnywhere, Category="Accessibility|Feedback")
	bool bEnableDamageNumbers = true;

	/** Small camera displacement on successful player hits. */
	UPROPERTY(Config, EditAnywhere, Category="Accessibility|Feedback")
	bool bEnableCameraShake = true;

	UPROPERTY(Config, EditAnywhere, Category="Accessibility|Feedback")
	bool bEnableImpactSounds = true;

	UPROPERTY(Config, EditAnywhere, Category="Accessibility|Feedback", meta=(ClampMin="0.0", ClampMax="1.0"))
	float CameraShakeStrength = 0.35f;

	/** Scales nonessential impact/death particles. Zero disables them. */
	UPROPERTY(Config, EditAnywhere, Category="Accessibility|Feedback", meta=(ClampMin="0.0", ClampMax="1.0"))
	float EffectsDensity = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category="Accessibility|Feedback", meta=(ClampMin="0.0", ClampMax="1.0"))
	float EffectsVolume = 0.65f;
};
