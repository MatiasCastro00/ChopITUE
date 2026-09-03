#include "Camera/ChopItCameraComponent.h"

#include "Camera/ChopItCameraAnchor.h"
#include "Camera/ChopItCameraCue.h"
#include "Camera/ChopItCameraUserSettings.h"
#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "CineCameraComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Core/CameraShakeAsset.h"
#include "Core/CameraAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
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

void FChopItOccludedPrimitiveState::CaptureAndApplyOcclusion(
	UPrimitiveComponent& Primitive,
	UMaterialInterface& InOcclusionMaterial)
{
	Component = &Primitive;
	OriginalMaterials.Reset();
	const int32 MaterialCount = Primitive.GetNumMaterials();
	OriginalMaterials.Reserve(MaterialCount);
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		OriginalMaterials.Add(Primitive.GetMaterial(MaterialIndex));
		Primitive.SetMaterial(MaterialIndex, &InOcclusionMaterial);
	}

	if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(&Primitive))
	{
		OriginalOverlayMaterial = MeshComponent->GetOverlayMaterial();
		MeshComponent->SetOverlayMaterial(nullptr);
	}
}

void FChopItOccludedPrimitiveState::Restore() const
{
	UPrimitiveComponent* Primitive = Component.Get();
	if (!IsValid(Primitive)) return;

	for (int32 MaterialIndex = 0; MaterialIndex < OriginalMaterials.Num(); ++MaterialIndex)
	{
		Primitive->SetMaterial(MaterialIndex, OriginalMaterials[MaterialIndex]);
	}
	if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(Primitive))
	{
		MeshComponent->SetOverlayMaterial(OriginalOverlayMaterial);
	}
}

UChopItCameraComponent::UChopItCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	SetAbsolute(true, true, false);
	static ConstructorHelpers::FObjectFinder<UCameraAsset> CameraAssetFinder(TEXT("/Game/ChopIt/Presentation/Camera/CA_PlayerCameras.CA_PlayerCameras"));
	if (CameraAssetFinder.Succeeded()) CameraReference.SetCameraAsset(CameraAssetFinder.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> OcclusionMaterialFinder(TEXT("/Game/ChopIt/Presentation/Camera/Materials/M_CameraFoliageOcclusion.M_CameraFoliageOcclusion"));
	if (OcclusionMaterialFinder.Succeeded()) OcclusionMaterial = OcclusionMaterialFinder.Object;
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
	ConfigureWorldCameraCollision();
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
	RestoreOcclusionMaterials();
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
	const FTransform ScriptedTarget = GetComponentTransform();
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (UCineCameraComponent* Output = GetOutputCameraComponent())
	{
		// Gameplay Cameras writes its evaluated pose during Super::TickComponent. Scripted
		// dialogue anchors are external to that graph, so apply their pose afterwards.
		const FTransform TargetTransform = ActiveMode == EChopItCameraMode::GameplayOrbit
			? Output->GetComponentTransform()
			: ScriptedTarget;
		const float RealDeltaTime = FMath::Clamp(
			DeltaTime > KINDA_SMALL_NUMBER ? DeltaTime : static_cast<float>(FApp::GetDeltaTime()),
			1.0f / 120.0f,
			1.0f / 20.0f);
		ApplyRenderedCameraPose(RealDeltaTime, TargetTransform, ActiveFieldOfView);
	}
	UpdateOcclusionTransparency();
}

FChopItCameraHandle UChopItCameraComponent::NewHandle() const
{
	FChopItCameraHandle Handle;
	Handle.Id = FGuid::NewGuid();
	return Handle;
}

FChopItCameraHandle UChopItCameraComponent::PushCue(
	const UChopItCameraCue* Cue,
	AChopItCameraAnchor* Anchor,
	AActor* Subject,
	const float FieldOfViewOverride,
	const float BlendInTimeOverride)
{
	if (!IsValid(Cue)) return {};
	if (CueRequests.IsEmpty()) SavedGameplayView = GameplayView;
	FChopItCameraHandle Handle = NewHandle();
	FCueRequest& Request = CueRequests.Add(Handle.Id);
	Request.Cue = Cue; Request.Anchor = Anchor; Request.Subject = Subject; Request.Sequence = NextSequence++;
	Request.FieldOfViewOverride = FieldOfViewOverride;
	Request.BlendInTimeOverride = BlendInTimeOverride;
	Request.bRequiresAnchor = Anchor != nullptr; Request.bRequiresSubject = Subject != nullptr;
	if (Cue->DurationPolicy == EChopItCameraDurationPolicy::Timed && Cue->Duration > 0.0f && GetWorld())
	{
		Request.ExpiresAt = GetWorld()->GetTimeSeconds() + Cue->Duration;
	}
	if (Cue->AssociatedVisualRig) Request.AssociatedRig = StartVisualCameraModifierRig(Cue->AssociatedVisualRig, Cue->Priority);
	ResolveActiveCue();
	UE_LOG(LogChopIt, Display,
		TEXT("Camera cue queued: cue=%s sequence=%llu mode=%d anchor=%s subject=%s paused=%d componentPauseTick=%d."),
		*GetNameSafe(Cue),
		Request.Sequence,
		static_cast<int32>(Cue->Mode),
		*GetNameSafe(Anchor),
		*GetNameSafe(Subject),
		GetWorld() && GetWorld()->IsPaused(),
		PrimaryComponentTick.bTickEvenWhenPaused);
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
	FShakeRequest& Request = ShakeRequests.Add(Handle.Id);
	Request.Scale = Scale;
	Request.Shake = StartCameraShakeAsset(ShakeAsset, Scale);
	return Handle;
}

void UChopItCameraComponent::StopRequest(const FChopItCameraHandle Handle, const bool bImmediate)
{
	if (ExternalInputLocks.Contains(Handle.Id)) { PopInputLock(Handle); return; }
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

FChopItCameraHandle UChopItCameraComponent::PushInputLock(const EChopItCameraInputLock Locks)
{
	if (Locks == EChopItCameraInputLock::None) return {};
	const FChopItCameraHandle Handle = NewHandle();
	ExternalInputLocks.Add(Handle.Id, Locks);
	ResolveActiveCue();
	return Handle;
}

void UChopItCameraComponent::PopInputLock(const FChopItCameraHandle Handle)
{
	if (ExternalInputLocks.Remove(Handle.Id) > 0) ResolveActiveCue();
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
	EChopItCameraInputLock RequestedLocks = EChopItCameraInputLock::None;
	for (const TPair<FGuid, EChopItCameraInputLock>& Pair : ExternalInputLocks) RequestedLocks |= Pair.Value;
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
		ActiveCueSequence = 0;
		ActiveMode = EChopItCameraMode::GameplayOrbit; ActiveLocks = RequestedLocks; ActiveAnchor.Reset(); ActiveSubject.Reset();
		if (const UChopItCameraUserSettings* Settings = Cast<UChopItCameraUserSettings>(UGameUserSettings::GetGameUserSettings())) ActiveFieldOfView = Settings->FieldOfView;
		return;
	}
	const UChopItCameraCue* Cue = Winner->Cue.Get();
	ActiveCueSequence = Winner->Sequence;
	ActiveMode = Cue->Mode;
	ActiveLocks = static_cast<EChopItCameraInputLock>(Cue->InputLocks) | RequestedLocks;
	ActiveAnchor = Winner->Anchor;
	ActiveSubject = Winner->Subject.IsValid() ? Winner->Subject : (Winner->Anchor.IsValid() ? Winner->Anchor->DefaultSubject : nullptr);
	ActiveFieldOfView = Winner->FieldOfViewOverride > 0.0f ? Winner->FieldOfViewOverride : Cue->FieldOfView;
	ActiveBlendInTime = Winner->BlendInTimeOverride >= 0.0f
		? Winner->BlendInTimeOverride
		: FMath::Max(0.0f, Cue->BlendInTime);
	LastCueBlendOutTime = FMath::Max(0.0f, Cue->BlendOutTime);
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
	if (ActiveSubject.IsValid())
	{
		const FVector BaseFocus = ActiveAnchor->bUseActorLocationForSubject
			? ActiveSubject->GetActorLocation()
			: ResolveSubjectFocus(ActiveSubject.Get());
		const FVector Focus = BaseFocus + ActiveAnchor->SubjectFocusOffset;
		Rotation = (Focus - Location).Rotation();
	}
	SetWorldLocationAndRotation(Location, Rotation);
}

FVector UChopItCameraComponent::ResolveSubjectFocus(const AActor* Subject) const
{
	if (!IsValid(Subject)) return FVector::ZeroVector;
	const FBox SubjectBounds = Subject->GetComponentsBoundingBox(true);
	return SubjectBounds.IsValid ? SubjectBounds.GetCenter() : Subject->GetActorLocation() + FVector::UpVector * PivotHeight;
}

void UChopItCameraComponent::ApplyRenderedCameraPose(
	const float DeltaTime,
	const FTransform& TargetTransform,
	const float TargetFieldOfView)
{
	UCineCameraComponent* Output = GetOutputCameraComponent();
	if (!Output) return;

	if (!bHasRenderedCameraPose)
	{
		LastRenderedTransform = TargetTransform;
		LastRenderedFieldOfView = TargetFieldOfView;
		LastRenderedMode = ActiveMode;
		LastRenderedCueSequence = ActiveCueSequence;
		LastRenderedAnchor = ActiveAnchor;
		LastRenderedSubject = ActiveSubject;
		bHasRenderedCameraPose = true;
	}

	const bool bTargetChanged = LastRenderedMode != ActiveMode
		|| LastRenderedCueSequence != ActiveCueSequence
		|| LastRenderedAnchor != ActiveAnchor
		|| LastRenderedSubject != ActiveSubject;
	if (bTargetChanged)
	{
		BlendStartTransform = LastRenderedTransform;
		BlendStartFieldOfView = LastRenderedFieldOfView;
		PoseBlendElapsed = 0.0f;
		PoseBlendDuration = ActiveMode == EChopItCameraMode::GameplayOrbit
			? LastCueBlendOutTime
			: ActiveBlendInTime;
		LastRenderedMode = ActiveMode;
		LastRenderedCueSequence = ActiveCueSequence;
		LastRenderedAnchor = ActiveAnchor;
		LastRenderedSubject = ActiveSubject;
		UE_LOG(LogChopIt, Display,
			TEXT("Camera transition started: mode=%d sequence=%llu duration=%.2f from=%s to=%s paused=%d."),
			static_cast<int32>(ActiveMode),
			ActiveCueSequence,
			PoseBlendDuration,
			*BlendStartTransform.GetLocation().ToCompactString(),
			*TargetTransform.GetLocation().ToCompactString(),
			GetWorld() && GetWorld()->IsPaused());
	}

	float Alpha = 1.0f;
	if (PoseBlendDuration > KINDA_SMALL_NUMBER && PoseBlendElapsed < PoseBlendDuration)
	{
		PoseBlendElapsed = FMath::Min(PoseBlendDuration, PoseBlendElapsed + DeltaTime);
		const float LinearAlpha = PoseBlendElapsed / PoseBlendDuration;
		Alpha = FMath::SmoothStep(0.0f, 1.0f, LinearAlpha);
	}

	if (Alpha < 1.0f)
	{
		const FVector Location = FMath::Lerp(BlendStartTransform.GetLocation(), TargetTransform.GetLocation(), Alpha);
		const FQuat Rotation = FQuat::Slerp(BlendStartTransform.GetRotation(), TargetTransform.GetRotation(), Alpha).GetNormalized();
		LastRenderedTransform = FTransform(Rotation, Location);
		LastRenderedFieldOfView = FMath::Lerp(BlendStartFieldOfView, TargetFieldOfView, Alpha);
	}
	else
	{
		LastRenderedTransform = TargetTransform;
		LastRenderedFieldOfView = TargetFieldOfView;
	}

	FTransform OutputTransform = LastRenderedTransform;
	if (ActiveMode != EChopItCameraMode::GameplayOrbit && !ShakeRequests.IsEmpty())
	{
		// Gameplay Cameras evaluates shakes during Super::TickComponent, but ChopIt's
		// authored anchor pose is intentionally applied afterwards. Reapply a compact
		// real-time shake layer here so scripted dialogue shots retain their modifiers
		// while the world is paused and without accumulating error into the base blend.
		float CombinedScale = 0.0f;
		for (const TPair<FGuid, FShakeRequest>& Pair : ShakeRequests)
		{
			CombinedScale += Pair.Value.Scale;
		}
		CombinedScale = FMath::Clamp(CombinedScale, 0.0f, 2.5f);
		const double Now = FPlatformTime::Seconds();
		const float Side = (FMath::Sin(Now * 67.0) * 3.6f + FMath::Sin(Now * 31.0) * 1.8f) * CombinedScale;
		const float Up = (FMath::Cos(Now * 59.0) * 2.8f + FMath::Sin(Now * 43.0) * 1.4f) * CombinedScale;
		const float Forward = FMath::Sin(Now * 37.0) * 1.2f * CombinedScale;
		const FQuat BaseRotation = OutputTransform.GetRotation();
		OutputTransform.AddToTranslation(BaseRotation.RotateVector(FVector(Forward, Side, Up)));
		const FRotator AngularJitter(
			FMath::Sin(Now * 53.0) * 0.75f * CombinedScale,
			FMath::Cos(Now * 47.0) * 0.90f * CombinedScale,
			FMath::Sin(Now * 61.0) * 0.45f * CombinedScale);
		OutputTransform.SetRotation((BaseRotation * AngularJitter.Quaternion()).GetNormalized());
	}
	Output->SetWorldTransform(OutputTransform);
	Output->SetFieldOfView(LastRenderedFieldOfView);
}

void UChopItCameraComponent::ConfigureWorldCameraCollision()
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		if (!IsValid(Actor)) continue;

		const FString ActorName = Actor->GetName();
		// Name checks migrate existing generated maps. New authored surfaces use the tag.
		const bool bLegacyCameraSolid = ActorName.StartsWith(TEXT("Ground"))
			|| ActorName.StartsWith(TEXT("ChainLab_Ground"))
			|| ActorName.StartsWith(TEXT("Boundary_"))
			|| ActorName.StartsWith(TEXT("ChainLab_Boundary"));
		const bool bActorCameraSolid = Actor->ActorHasTag(ChopItCollisionChannels::CameraSolidTag) || bLegacyCameraSolid;

		TInlineComponentArray<UPrimitiveComponent*> Primitives(Actor);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (!IsValid(Primitive) || Primitive->GetCollisionEnabled() == ECollisionEnabled::NoCollision) continue;
			const bool bCameraSolid = bActorCameraSolid || Primitive->ComponentHasTag(ChopItCollisionChannels::CameraSolidTag);
			Primitive->SetCollisionResponseToChannel(
				ChopItCollisionChannels::CameraSolid,
				bCameraSolid ? ECR_Block : ECR_Ignore);
		}
	}
}

void UChopItCameraComponent::UpdateOcclusionTransparency()
{
	// Authored dialogue/cinematic shots deliberately place the camera around their
	// focal actors. Applying the gameplay foliage corridor here can replace pieces
	// of the focal set (notably the quota oven) with the dither material.
	if (ActiveMode != EChopItCameraMode::GameplayOrbit)
	{
		RestoreOcclusionMaterials();
		return;
	}

	UWorld* World = GetWorld();
	AActor* CameraTarget = ActiveSubject.IsValid() ? ActiveSubject.Get() : GetOwner();
	if (!World || !IsValid(CameraTarget) || !IsValid(OcclusionMaterial))
	{
		RestoreOcclusionMaterials();
		return;
	}

	const FVector TargetLocation = CameraTarget->GetActorLocation() + FVector(0.0, 0.0, PivotHeight);
	const UCineCameraComponent* OutputCamera = GetOutputCameraComponent();
	const FVector CameraLocation = OutputCamera ? OutputCamera->GetComponentLocation() : GetComponentLocation();
	const FVector Segment = TargetLocation - CameraLocation;
	const float SegmentLength = Segment.Size();
	if (SegmentLength <= KINDA_SMALL_NUMBER)
	{
		RestoreOcclusionMaterials();
		return;
	}

	// Sample the subject's screen-facing silhouette instead of using a constant-width
	// capsule. These rays converge at the camera, so nearby lateral objects do not fade.
	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQuery.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams BaseQuery(SCENE_QUERY_STAT(ChopItCameraOcclusion), false);
	BaseQuery.AddIgnoredActor(GetOwner());
	if (CameraTarget != GetOwner()) BaseQuery.AddIgnoredActor(CameraTarget);

	const float SilhouetteRadius = FMath::Max(1.0f, OcclusionCorridorRadius);
	const FVector CameraRight = OutputCamera ? OutputCamera->GetRightVector() : GetRightVector();
	const FVector CameraUp = OutputCamera ? OutputCamera->GetUpVector() : GetUpVector();
	static const FVector2D SilhouetteSamples[] =
	{
		FVector2D(0.0, 0.0),
		FVector2D(-1.0, 0.0), FVector2D(1.0, 0.0),
		FVector2D(0.0, -1.0), FVector2D(0.0, 1.0),
		FVector2D(-0.7, -0.7), FVector2D(0.7, -0.7),
		FVector2D(-0.7, 0.7), FVector2D(0.7, 0.7)
	};
	TSet<AActor*> OccludingActors;
	for (const FVector2D& Sample : SilhouetteSamples)
	{
		const FVector SampleTarget = TargetLocation
			+ CameraRight * (Sample.X * SilhouetteRadius)
			+ CameraUp * (Sample.Y * SilhouetteRadius * 1.35f);
		FCollisionQueryParams RayQuery = BaseQuery;
		// Continue behind each hit so multiple aligned props can fade simultaneously.
		for (int32 Layer = 0; Layer < 8; ++Layer)
		{
			FHitResult Hit;
			if (!World->LineTraceSingleByObjectType(Hit, CameraLocation, SampleTarget, ObjectQuery, RayQuery)) break;
			AActor* HitActor = Hit.GetActor();
			if (!IsValid(HitActor)) break;
			OccludingActors.Add(HitActor);
			RayQuery.AddIgnoredActor(HitActor);
		}
	}

	TSet<UPrimitiveComponent*> CurrentOccluders;
	auto AddRenderablePrimitive = [this, &CurrentOccluders](UPrimitiveComponent* Primitive)
	{
		if (!IsValid(Primitive) || !Primitive->IsRegistered() || !Primitive->IsVisible() || Primitive->GetNumMaterials() <= 0
			|| Primitive->GetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid) == ECR_Block)
		{
			return;
		}
		CurrentOccluders.Add(Primitive);
		const bool bAlreadyOccluded = OccludedPrimitives.ContainsByPredicate(
			[Primitive](const FChopItOccludedPrimitiveState& State) { return State.Component.Get() == Primitive; });
		if (bAlreadyOccluded) return;

		FChopItOccludedPrimitiveState& State = OccludedPrimitives.AddDefaulted_GetRef();
		State.CaptureAndApplyOcclusion(*Primitive, *OcclusionMaterial);
	};

	for (AActor* HitActor : OccludingActors)
	{
		if (!IsValid(HitActor) || HitActor == GetOwner() || HitActor == CameraTarget) continue;

		// Occlusion is actor-wide: a tree or compound prop must not leave its trunk or
		// another visual piece opaque merely because the corridor touched its crown first.
		TInlineComponentArray<UPrimitiveComponent*> RenderPrimitives(HitActor);
		for (UPrimitiveComponent* Primitive : RenderPrimitives) AddRenderablePrimitive(Primitive);
	}

	for (int32 StateIndex = OccludedPrimitives.Num() - 1; StateIndex >= 0; --StateIndex)
	{
		FChopItOccludedPrimitiveState& State = OccludedPrimitives[StateIndex];
		UPrimitiveComponent* Primitive = State.Component.Get();
		if (IsValid(Primitive) && CurrentOccluders.Contains(Primitive)) continue;
		State.Restore();
		OccludedPrimitives.RemoveAtSwap(StateIndex, 1, EAllowShrinking::No);
	}
}

void UChopItCameraComponent::RestoreOcclusionMaterials()
{
	for (FChopItOccludedPrimitiveState& State : OccludedPrimitives)
	{
		State.Restore();
	}
	OccludedPrimitives.Reset();
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
