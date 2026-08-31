#include "Camera/ChopItCameraUserSettings.h"

void UChopItCameraUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	MouseSensitivity = 1.0f;
	GamepadYawSpeed = 160.0f;
	GamepadPitchSpeed = 120.0f;
	bInvertVertical = false;
	bEnableSmoothing = false;
	FieldOfView = 85.0f;
	PreferredDistance = 850.0f;
	ShakeStrength = 1.0f;
}
