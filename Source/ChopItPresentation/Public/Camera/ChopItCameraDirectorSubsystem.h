#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "Camera/ChopItCameraTypes.h"
#include "ChopItCameraDirectorSubsystem.generated.h"

class AChopItCameraAnchor;
class UCameraShakeAsset;
class UChopItCameraComponent;
class UChopItCameraCue;
class UChopItCameraEffectPreset;

UCLASS()
class CHOPITPRESENTATION_API UChopItCameraDirectorSubsystem final : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="ChopIt|Camera") FChopItCameraHandle PushCameraCue(const UChopItCameraCue* Cue, AChopItCameraAnchor* Anchor, AActor* Subject);
	UFUNCTION(BlueprintCallable, Category="ChopIt|Camera") void PopCameraCue(FChopItCameraHandle Handle, bool bImmediate = false);
	UFUNCTION(BlueprintCallable, Category="ChopIt|Camera", meta=(AdvancedDisplay="DurationOverride")) FChopItCameraHandle PushCameraEffect(const UChopItCameraEffectPreset* Preset, float DurationOverride = -1.0f);
	UFUNCTION(BlueprintCallable, Category="ChopIt|Camera", meta=(AdvancedDisplay="WorldOrigin")) FChopItCameraHandle PlayCameraShake(const UCameraShakeAsset* ShakeAsset, float Scale = 1.0f, FVector WorldOrigin = FVector::ZeroVector);
	UFUNCTION(BlueprintCallable, Category="ChopIt|Camera") void PopCameraEffect(FChopItCameraHandle Handle, bool bImmediate = false);
	UFUNCTION(BlueprintCallable, Category="ChopIt|Camera") void StopCameraRequest(FChopItCameraHandle Handle);
	UFUNCTION(BlueprintCallable, Category="ChopIt|Camera") void ResetGameplayCamera();
	UFUNCTION(BlueprintPure, Category="ChopIt|Camera") bool IsInputLocked(EChopItCameraInputLock Lock) const;
	UChopItCameraComponent* GetCameraComponent() const;
};
