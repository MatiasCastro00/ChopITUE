#pragma once

#include "GameFramework/GameUserSettings.h"
#include "ChopItCameraUserSettings.generated.h"

UCLASS(Config=GameUserSettings)
class CHOPITPRESENTATION_API UChopItCameraUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
public:
	virtual void SetToDefaults() override;

	UPROPERTY(Config, BlueprintReadWrite, Category="Camera", meta=(ClampMin="0.05", ClampMax="5")) float MouseSensitivity = 1.0f;
	UPROPERTY(Config, BlueprintReadWrite, Category="Camera", meta=(ClampMin="20", ClampMax="500")) float GamepadYawSpeed = 160.0f;
	UPROPERTY(Config, BlueprintReadWrite, Category="Camera", meta=(ClampMin="20", ClampMax="500")) float GamepadPitchSpeed = 120.0f;
	UPROPERTY(Config, BlueprintReadWrite, Category="Camera") bool bInvertVertical = false;
	UPROPERTY(Config, BlueprintReadWrite, Category="Camera") bool bEnableSmoothing = false;
	UPROPERTY(Config, BlueprintReadWrite, Category="Camera", meta=(ClampMin="60", ClampMax="120")) float FieldOfView = 85.0f;
	UPROPERTY(Config, BlueprintReadWrite, Category="Camera", meta=(ClampMin="550", ClampMax="1400")) float PreferredDistance = 850.0f;
	UPROPERTY(Config, BlueprintReadWrite, Category="Camera", meta=(ClampMin="0", ClampMax="2")) float ShakeStrength = 1.0f;
};
