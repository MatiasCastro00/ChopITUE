#pragma once

#include "Engine/DataAsset.h"
#include "Camera/ChopItCameraTypes.h"
#include "ChopItCameraCue.generated.h"

class UCameraRigAsset;

UCLASS(BlueprintType)
class CHOPITPRESENTATION_API UChopItCameraCue : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") EChopItCameraMode Mode = EChopItCameraMode::Scripted;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") TObjectPtr<UCameraRigAsset> CameraRig;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta=(ClampMin="0", ClampMax="1000")) int32 Priority = 200;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") EChopItCameraDurationPolicy DurationPolicy = EChopItCameraDurationPolicy::Manual;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta=(ClampMin="0")) float Duration = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition", meta=(ClampMin="0")) float BlendInTime = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition", meta=(ClampMin="0")) float BlendOutTime = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta=(ClampMin="5", ClampMax="170")) float FieldOfView = 65.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta=(Bitmask, BitmaskEnum="/Script/ChopItPresentation.EChopItCameraInputLock")) int32 InputLocks = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") TObjectPtr<UCameraRigAsset> AssociatedVisualRig;
};

UCLASS(BlueprintType)
class CHOPITPRESENTATION_API UChopItCameraEffectPreset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") TObjectPtr<UCameraRigAsset> ModifierRig;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta=(ClampMin="0")) float DefaultDuration = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition", meta=(ClampMin="0")) float FadeInTime = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition", meta=(ClampMin="0")) float FadeOutTime = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera") int32 OrderKey = 0;
};
