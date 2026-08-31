#include "Camera/ChopItCameraDirectorSubsystem.h"
#include "Camera/ChopItCameraComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

UChopItCameraComponent* UChopItCameraDirectorSubsystem::GetCameraComponent() const
{
	const APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	return PC && PC->GetPawn() ? PC->GetPawn()->FindComponentByClass<UChopItCameraComponent>() : nullptr;
}

FChopItCameraHandle UChopItCameraDirectorSubsystem::PushCameraCue(const UChopItCameraCue* Cue, AChopItCameraAnchor* Anchor, AActor* Subject) { if (UChopItCameraComponent* C = GetCameraComponent()) return C->PushCue(Cue, Anchor, Subject); return {}; }
void UChopItCameraDirectorSubsystem::PopCameraCue(FChopItCameraHandle Handle, bool bImmediate) { if (UChopItCameraComponent* C = GetCameraComponent()) C->PopCue(Handle, bImmediate); }
FChopItCameraHandle UChopItCameraDirectorSubsystem::PushCameraEffect(const UChopItCameraEffectPreset* Preset, float DurationOverride) { if (UChopItCameraComponent* C = GetCameraComponent()) return C->PushEffect(Preset, DurationOverride); return {}; }
FChopItCameraHandle UChopItCameraDirectorSubsystem::PlayCameraShake(const UCameraShakeAsset* ShakeAsset, float Scale, FVector WorldOrigin) { if (UChopItCameraComponent* C = GetCameraComponent()) return C->PlayShake(ShakeAsset, Scale, WorldOrigin); return {}; }
void UChopItCameraDirectorSubsystem::PopCameraEffect(FChopItCameraHandle Handle, bool bImmediate) { if (UChopItCameraComponent* C = GetCameraComponent()) C->StopRequest(Handle, bImmediate); }
void UChopItCameraDirectorSubsystem::StopCameraRequest(FChopItCameraHandle Handle) { if (UChopItCameraComponent* C = GetCameraComponent()) C->StopRequest(Handle); }
void UChopItCameraDirectorSubsystem::ResetGameplayCamera() { if (UChopItCameraComponent* C = GetCameraComponent()) C->ResetGameplayCamera(); }
bool UChopItCameraDirectorSubsystem::IsInputLocked(EChopItCameraInputLock Lock) const { if (const UChopItCameraComponent* C = GetCameraComponent()) return C->IsInputLocked(Lock); return false; }
