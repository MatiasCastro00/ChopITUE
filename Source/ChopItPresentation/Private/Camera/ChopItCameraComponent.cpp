#include "Camera/ChopItCameraComponent.h"

#include "Camera/ChopItCameraAnchor.h"
#include "Camera/ChopItCameraCue.h"
#include "Camera/ChopItCameraUserSettings.h"
#include "ChopItCollision.h"
#include "CineCameraComponent.h"
#include "Core/CameraShakeAsset.h"
#include "Core/CameraAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/ObjectSaveContext.h"

namespace
{
	constexpr float DefaultYaw = -45.0f;
	constexpr float DefaultPitch = -32.0f;
	constexpr float DefaultDistance = 850.0f;
	constexpr float PivotHeight = 120.0f;
	constexpr float ZoomBlendSeconds = 0.12f;
}

UChopItCameraComponent::UChopItCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	SetAbsolute(true, true, false);
	static ConstructorHelpers::FObjectFinder<UCameraAsset> CameraAssetFinder(TEXT("/Game/ChopIt/Presentation/Camera/CA_PlayerCameras.CA_PlayerCameras"));
	if (CameraAssetFinder.Succeeded()) CameraReference.SetCameraAsset(CameraAssetFinder.Object);
}

// UGameplayCameraComponent is MinimalAPI in 5.8 and does not export these virtual
// overrides. Re-declaring them here keeps the experimental plugin behind ChopIt's ABI.
void UChopItCameraComponent::OnRegister() { UGameplayCameraComponentBase::OnRegister(); }
void UChopItCameraComponent::OnUnregister() { USceneComponent::OnUnregister(); }
void UChopItCameraComponent::PostLoad() { USceneComponent::PostLoad(); }
void UChopItCameraComponent::PreSave(FObjectPreSaveContext ObjectSaveContext) { USceneComponent::PreSave(ObjectSaveContext); }
#if WITH_EDITOR
void UChopItCameraComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) { USceneComponent::PostEditChangeProperty(PropertyChangedEvent); }
#endif
UCameraAsset* UChopItCameraComponent::OnCreateEvaluationContext() { return CameraReference.GetCameraAsset(); }
void UChopItCameraComponent::OnUpdateEvaluationContext(bool) {}

void UChopItCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	if (const UChopItCameraUserSettings* Settings = Cast<UChopItCameraUserSettings>(UGameUserSettings::GetGameUserSettings()))
	{
		GameplayView.Distance = FMath::Clamp(Settings->PreferredDistance, MinDistance, MaxDistance);
		TargetDistance = GameplayView.Distance;
		CurrentCollisionDistance = GameplayView.Distance;
		ActiveFieldOfView = Settings->FieldOfView;
	}
	ResetGameplayCamera();
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			ActivateCameraForPlayerController(PC, true);
		}
	}
}

void UChopItCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const TPair<FGuid, FEffectRequest>& Pair : EffectRequests)
	{
		if (Pair.Value.Rig.IsValid()) StopCameraModifierRig(Pair.Value.Rig, true);
	}
	for (const TPair<FGuid, FShakeRequest>& Pair : ShakeRequests)
	{
		if (Pair.Value.Shake.IsValid()) StopCameraShakeAsset(Pair.Value.Shake, true);
	}
	CueRequests.Empty(); EffectRequests.Empty(); ShakeRequests.Empty();
	Super::EndPlay(EndPlayReason);
}

void UChopItCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	PruneInvalidRequests();
	ResolveActiveCue();
	if (ActiveMode == EChopItCameraMode::GameplayOrbit) UpdateGameplayTransform(DeltaTime);
	else UpdateScriptedTransform();
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (UCineCameraComponent* Output = GetOutputCameraComponent()) Output->SetFieldOfView(ActiveFieldOfView);
}

FChopItCameraHandle UChopItCameraComponent::NewHandle() const
{
	FChopItCameraHandle Handle;
	Handle.Id = FGuid::NewGuid();
	return Handle;
}

FChopItCameraHandle UChopItCameraComponent::PushCue(const UChopItCameraCue* Cue, AChopItCameraAnchor* Anchor, AActor* Subject)
{
	if (!IsValid(Cue)) return {};
	if (CueRequests.IsEmpty()) SavedGameplayView = GameplayView;
	FChopItCameraHandle Handle = NewHandle();
	FCueRequest& Request = CueRequests.Add(Handle.Id);
	Request.Cue = Cue; Request.Anchor = Anchor; Request.Subject = Subject; Request.Sequence = NextSequence++;
	Request.bRequiresAnchor = Anchor != nullptr; Request.bRequiresSubject = Subject != nullptr;
	if (Cue->DurationPolicy == EChopItCameraDurationPolicy::Timed && Cue->Duration > 0.0f && GetWorld())
	{
		Request.ExpiresAt = GetWorld()->GetTimeSeconds() + Cue->Duration;
	}
	if (Cue->AssociatedVisualRig) Request.AssociatedRig = StartVisualCameraModifierRig(Cue->AssociatedVisualRig, Cue->Priority);
	ResolveActiveCue();
	return Handle;
}

void UChopItCameraComponent::PopCue(const FChopItCameraHandle Handle, const bool bImmediate)
{
	if (FCueRequest* Request = CueRequests.Find(Handle.Id))
	{
		if (Request->AssociatedRig.IsValid()) StopCameraModifierRig(Request->AssociatedRig, bImmediate);
		CueRequests.Remove(Handle.Id);
		ResolveActiveCue();
	}
}

FChopItCameraHandle UChopItCameraComponent::PushEffect(const UChopItCameraEffectPreset* Preset, const float DurationOverride)
{
	if (!IsValid(Preset) || Preset->ModifierRig == nullptr) return {};
	FChopItCameraHandle Handle = NewHandle();
	FEffectRequest& Request = EffectRequests.Add(Handle.Id);
	Request.Rig = StartVisualCameraModifierRig(Preset->ModifierRig, Preset->OrderKey);
	const float Duration = DurationOverride >= 0.0f ? DurationOverride : Preset->DefaultDuration;
	if (Duration > 0.0f && GetWorld()) Request.ExpiresAt = GetWorld()->GetTimeSeconds() + Duration;
	return Handle;
}

FChopItCameraHandle UChopItCameraComponent::PlayShake(const UCameraShakeAsset* ShakeAsset, float Scale, const FVector& WorldOrigin)
{
	if (!IsValid(ShakeAsset)) return {};
	if (!WorldOrigin.IsNearlyZero() && GetOwner())
	{
		Scale *= FMath::Clamp(1.0f - FVector::Distance(WorldOrigin, GetOwner()->GetActorLocation()) / 2500.0f, 0.0f, 1.0f);
	}
	if (const UChopItCameraUserSettings* Settings = Cast<UChopItCameraUserSettings>(UGameUserSettings::GetGameUserSettings())) Scale *= Settings->ShakeStrength;
	if (Scale <= KINDA_SMALL_NUMBER) return {};
	FChopItCameraHandle Handle = NewHandle();
	ShakeRequests.Add(Handle.Id).Shake = StartCameraShakeAsset(ShakeAsset, Scale);
	return Handle;
}

void UChopItCameraComponent::StopRequest(const FChopItCameraHandle Handle, const bool bImmediate)
{
	if (CueRequests.Contains(Handle.Id)) { PopCue(Handle, bImmediate); return; }
	if (FEffectRequest* Effect = EffectRequests.Find(Handle.Id))
	{
		if (Effect->Rig.IsValid()) StopCameraModifierRig(Effect->Rig, bImmediate);
		EffectRequests.Remove(Handle.Id); return;
	}
	if (FShakeRequest* Shake = ShakeRequests.Find(Handle.Id))
	{
		if (Shake->Shake.IsValid()) StopCameraShakeAsset(Shake->Shake, bImmediate);
		ShakeRequests.Remove(Handle.Id);
	}
}

void UChopItCameraComponent::ResetGameplayCamera()
{
	GameplayView.Yaw = DefaultYaw;
	GameplayView.Pitch = DefaultPitch;
	float Preferred = DefaultDistance;
	if (const UChopItCameraUserSettings* Settings = Cast<UChopItCameraUserSettings>(UGameUserSettings::GetGameUserSettings()))
	{
		Preferred = Settings->PreferredDistance;
		ActiveFieldOfView = Settings->FieldOfView;
	}
	GameplayView.Distance = TargetDistance = CurrentCollisionDistance = FMath::Clamp(Preferred, MinDistance, MaxDistance);
}

void UChopItCameraComponent::AddLookInput(const FVector2D& Input, const float DeltaSeconds)
{
	AddGamepadLookInput(Input, DeltaSeconds);
}

void UChopItCameraComponent::AddMouseLookInput(const FVector2D& Input, const float DeltaSeconds)
{
	if (IsInputLocked(EChopItCameraInputLock::Camera) || ActiveMode != EChopItCameraMode::GameplayOrbit) return;
	float Sensitivity = 1.0f;
	bool bInvert = false;
	bool bSmooth = false;
	if (const UChopItCameraUserSettings* Settings = Cast<UChopItCameraUserSettings>(UGameUserSettings::GetGameUserSettings()))
	{
		Sensitivity = Settings->MouseSensitivity;
		bInvert = Settings->bInvertVertical;
		bSmooth = Settings->bEnableSmoothing;
	}
	SmoothedMouseInput = bSmooth
		? FMath::Vector2DInterpTo(SmoothedMouseInput, Input, DeltaSeconds, 20.0f)
		: Input;
	GameplayView.Yaw = FMath::UnwindDegrees(GameplayView.Yaw + SmoothedMouseInput.X * Sensitivity);
	const float Vertical = bInvert ? -SmoothedMouseInput.Y : SmoothedMouseInput.Y;
	GameplayView.Pitch = FMath::Clamp(GameplayView.Pitch + Vertical * Sensitivity, MinPitch, MaxPitch);
}

void UChopItCameraComponent::AddGamepadLookInput(const FVector2D& Input, const float DeltaSeconds)
{
	if (IsInputLocked(EChopItCameraInputLock::Camera) || ActiveMode != EChopItCameraMode::GameplayOrbit) return;
	float YawSpeed = 160.0f;
	float PitchSpeed = 120.0f;
	bool bInvert = false;
	if (const UChopItCameraUserSettings* Settings = Cast<UChopItCameraUserSettings>(UGameUserSettings::GetGameUserSettings()))
	{
		YawSpeed = Settings->GamepadYawSpeed;
		PitchSpeed = Settings->GamepadPitchSpeed;
		bInvert = Settings->bInvertVertical;
	}
	GameplayView.Yaw = FMath::UnwindDegrees(GameplayView.Yaw + Input.X * YawSpeed * DeltaSeconds);
	const float Vertical = bInvert ? -Input.Y : Input.Y;
	GameplayView.Pitch = FMath::Clamp(GameplayView.Pitch + Vertical * PitchSpeed * DeltaSeconds, MinPitch, MaxPitch);
}

void UChopItCameraComponent::AddZoomInput(const float Input)
{
	if (FMath::IsNearlyZero(Input) || IsInputLocked(EChopItCameraInputLock::Camera) || ActiveMode != EChopItCameraMode::GameplayOrbit) return;
	TargetDistance = FMath::Clamp(TargetDistance - FMath::Sign(Input) * ZoomStep, MinDistance, MaxDistance);
	if (UChopItCameraUserSettings* Settings = Cast<UChopItCameraUserSettings>(UGameUserSettings::GetGameUserSettings()))
	{
		Settings->PreferredDistance = TargetDistance;
		Settings->SaveSettings();
	}
}

bool UChopItCameraComponent::IsInputLocked(const EChopItCameraInputLock Lock) const
{
	return EnumHasAnyFlags(ActiveLocks, Lock);
}

void UChopItCameraComponent::ResolveActiveCue()
{
	const FCueRequest* Winner = nullptr;
	for (const TPair<FGuid, FCueRequest>& Pair : CueRequests)
	{
		const UChopItCameraCue* Cue = Pair.Value.Cue.Get();
		if (!Cue) continue;
		if (!Winner || Cue->Priority > Winner->Cue->Priority || (Cue->Priority == Winner->Cue->Priority && Pair.Value.Sequence > Winner->Sequence)) Winner = &Pair.Value;
	}
	if (!Winner)
	{
		if (SavedGameplayView.IsSet()) { GameplayView = SavedGameplayView.GetValue(); TargetDistance = GameplayView.Distance; SavedGameplayView.Reset(); }
		ActiveMode = EChopItCameraMode::GameplayOrbit; ActiveLocks = EChopItCameraInputLock::None; ActiveAnchor.Reset(); ActiveSubject.Reset();
		if (const UChopItCameraUserSettings* Settings = Cast<UChopItCameraUserSettings>(UGameUserSettings::GetGameUserSettings())) ActiveFieldOfView = Settings->FieldOfView;
		return;
	}
	const UChopItCameraCue* Cue = Winner->Cue.Get();
	ActiveMode = Cue->Mode;
	ActiveLocks = static_cast<EChopItCameraInputLock>(Cue->InputLocks);
	ActiveAnchor = Winner->Anchor;
	ActiveSubject = Winner->Subject.IsValid() ? Winner->Subject : (Winner->Anchor.IsValid() ? Winner->Anchor->DefaultSubject : nullptr);
	ActiveFieldOfView = Cue->FieldOfView;
}

void UChopItCameraComponent::UpdateGameplayTransform(const float DeltaTime)
{
	GameplayView.Distance = FMath::FInterpTo(GameplayView.Distance, TargetDistance, DeltaTime, 1.0f / ZoomBlendSeconds);
	const FVector Pivot = GetOwner()->GetActorLocation() + FVector(0, 0, PivotHeight);
	const FRotator Rotation(GameplayView.Pitch, GameplayView.Yaw, 0.0f);
	const FVector DesiredLocation = Pivot - Rotation.Vector() * GameplayView.Distance;
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ChopItCameraCollision), false, GetOwner());
	float AllowedDistance = GameplayView.Distance;
	if (GetWorld() && GetWorld()->SweepSingleByChannel(Hit, Pivot, DesiredLocation, FQuat::Identity, ChopItCollisionChannels::CameraSolid, FCollisionShape::MakeSphere(18.0f), Query))
	{
		AllowedDistance = FMath::Max(80.0f, Hit.Distance - 12.0f);
	}
	CurrentCollisionDistance = AllowedDistance < CurrentCollisionDistance ? AllowedDistance : FMath::FInterpTo(CurrentCollisionDistance, AllowedDistance, DeltaTime, 10.0f);
	SetWorldLocationAndRotation(Pivot - Rotation.Vector() * CurrentCollisionDistance, Rotation);
	if (APawn* Pawn = Cast<APawn>(GetOwner())) if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController())) PC->SetControlRotation(Rotation);
}

void UChopItCameraComponent::UpdateScriptedTransform()
{
	if (!ActiveAnchor.IsValid()) return;
	FVector Location = ActiveAnchor->CameraTransform->GetComponentLocation();
	FRotator Rotation = ActiveAnchor->CameraTransform->GetComponentRotation();
	if (ActiveSubject.IsValid()) Rotation = (ActiveSubject->GetActorLocation() - Location).Rotation();
	SetWorldLocationAndRotation(Location, Rotation);
}

void UChopItCameraComponent::PruneInvalidRequests()
{
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	TArray<FGuid> Remove;
	for (const TPair<FGuid, FCueRequest>& Pair : CueRequests)
		if (!Pair.Value.Cue.IsValid() || (Pair.Value.ExpiresAt > 0.0 && Now >= Pair.Value.ExpiresAt) || (Pair.Value.bRequiresAnchor && !Pair.Value.Anchor.IsValid()) || (Pair.Value.bRequiresSubject && !Pair.Value.Subject.IsValid())) Remove.Add(Pair.Key);
	for (const FGuid& Id : Remove) StopRequest(FChopItCameraHandle{Id});
	Remove.Reset();
	for (const TPair<FGuid, FEffectRequest>& Pair : EffectRequests) if (Pair.Value.ExpiresAt > 0.0 && Now >= Pair.Value.ExpiresAt) Remove.Add(Pair.Key);
	for (const FGuid& Id : Remove) StopRequest(FChopItCameraHandle{Id});
	Remove.Reset();
	for (const TPair<FGuid, FShakeRequest>& Pair : ShakeRequests) if (!IsCameraShakeAssetPlaying(Pair.Value.Shake)) Remove.Add(Pair.Key);
	for (const FGuid& Id : Remove) ShakeRequests.Remove(Id);
}
