#include "Visual/ChopItVisualStyleComponent.h"

#include "Combat/ChopItHealthComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Enemies/ChopItEnemyCharacter.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Interaction/ChopItInteractable.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/ChopItCharacter.h"
#include "UnrealClient.h"

namespace ChopItVisualStyle
{
	constexpr TCHAR MegabonkPresetPath[] = TEXT("/Game/ChopIt/Presentation/VisualStyle/Presets/DA_VisualPreset_Megabonk.DA_VisualPreset_Megabonk");
	constexpr TCHAR MachinePartyPresetPath[] = TEXT("/Game/ChopIt/Presentation/VisualStyle/Presets/DA_VisualPreset_MachineParty.DA_VisualPreset_MachineParty");
	constexpr TCHAR HybridPresetPath[] = TEXT("/Game/ChopIt/Presentation/VisualStyle/Presets/DA_VisualPreset_Hybrid.DA_VisualPreset_Hybrid");
	constexpr TCHAR PostProcessMaterialPath[] = TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/M_PP_ChopItVisualStyle.M_PP_ChopItVisualStyle");
	constexpr TCHAR GlobalOutlineMaterialPath[] = TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Outline/MI_Outline_Global.MI_Outline_Global");
	constexpr TCHAR PlayerOutlineMaterialPath[] = TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Outline/MI_Outline_Player.MI_Outline_Player");
	constexpr TCHAR EnemyOutlineMaterialPath[] = TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Outline/MI_Outline_Enemy.MI_Outline_Enemy");
	constexpr TCHAR InteractableOutlineMaterialPath[] = TEXT("/Game/ChopIt/Presentation/VisualStyle/Materials/Outline/MI_Outline_Interactable.MI_Outline_Interactable");

	UChopItVisualStyleComponent* FindController(UWorld* World)
	{
		return World && World->GetGameState()
			? World->GetGameState()->FindComponentByClass<UChopItVisualStyleComponent>()
			: nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs SetPresetCommand(
		TEXT("ChopIt.Visual.Preset"),
		TEXT("Switch visual preset: Megabonk, MachineParty, or Hybrid."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UChopItVisualStyleComponent* Controller = FindController(World); Controller && Args.Num() > 0)
			{
				const FString Name = Args[0].Replace(TEXT(" "), TEXT(""));
				if (Name.Equals(TEXT("Megabonk"), ESearchCase::IgnoreCase)) Controller->ApplyPreset(EChopItVisualPreset::Megabonk);
				else if (Name.Equals(TEXT("MachineParty"), ESearchCase::IgnoreCase)) Controller->ApplyPreset(EChopItVisualPreset::MachineParty);
				else Controller->ApplyPreset(EChopItVisualPreset::Hybrid);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs SetIntensityCommand(
		TEXT("ChopIt.Visual.Intensity"),
		TEXT("Set global visual-style intensity in the 0..1 range."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UChopItVisualStyleComponent* Controller = FindController(World); Controller && Args.Num() > 0)
			{
				Controller->SetGlobalIntensity(FCString::Atof(*Args[0]));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs DamagePulseCommand(
		TEXT("ChopIt.Visual.Damage"),
		TEXT("Preview the temporary damage degradation pulse."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UChopItVisualStyleComponent* Controller = FindController(World))
			{
				Controller->TriggerDamageDegradation(Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1.0f);
			}
		}));
}

UChopItVisualStyleComponent::UChopItVisualStyleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
}

void UChopItVisualStyleComponent::BeginPlay()
{
	Super::BeginPlay();
	ConfigurePostProcess();

	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		HeightFogComponent = It->GetComponent();
		break;
	}

	if (UWorld* World = GetWorld())
	{
		ActorSpawnedHandle = World->AddOnActorSpawnedHandler(
			FOnActorSpawned::FDelegate::CreateUObject(this, &UChopItVisualStyleComponent::HandleActorSpawned));
	}

	if (UChopItCycleStateMachineComponent* Cycle = GetOwner()->FindComponentByClass<UChopItCycleStateMachineComponent>())
	{
		Cycle->OnPhaseChanged.AddUniqueDynamic(this, &UChopItVisualStyleComponent::HandleCyclePhaseChanged);
	}

	EChopItVisualPreset InitialPreset = DefaultPreset;
	FString RequestedPreset;
	if (FParse::Value(FCommandLine::Get(), TEXT("ChopItVisualPreset="), RequestedPreset))
	{
		if (RequestedPreset.Equals(TEXT("Megabonk"), ESearchCase::IgnoreCase)) InitialPreset = EChopItVisualPreset::Megabonk;
		else if (RequestedPreset.Equals(TEXT("MachineParty"), ESearchCase::IgnoreCase)) InitialPreset = EChopItVisualPreset::MachineParty;
		else InitialPreset = EChopItVisualPreset::Hybrid;
	}

	CurrentGlobalIntensity = DefaultGlobalIntensity;
	StartGlobalIntensity = DefaultGlobalIntensity;
	TargetGlobalIntensity = DefaultGlobalIntensity;
	float RequestedIntensity = DefaultGlobalIntensity;
	if (FParse::Value(FCommandLine::Get(), TEXT("ChopItVisualIntensity="), RequestedIntensity))
	{
		CurrentGlobalIntensity = FMath::Clamp(RequestedIntensity, 0.0f, 1.0f);
		StartGlobalIntensity = CurrentGlobalIntensity;
		TargetGlobalIntensity = CurrentGlobalIntensity;
	}
	FParse::Value(FCommandLine::Get(), TEXT("ChopItVisualCapture="), CaptureFilename);
	BasePreset = InitialPreset;
	CurrentValues = ValuesFromPreset(ResolvePreset(InitialPreset));
	StartValues = CurrentValues;
	TargetValues = CurrentValues;
	RefreshOutlinedActors();
	BindPlayerDamage();
	ApplyCurrentValues();
}

void UChopItVisualStyleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld(); World && ActorSpawnedHandle.IsValid())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}
	if (BoundPlayerHealth.IsValid())
	{
		BoundPlayerHealth->OnDamageReceived.RemoveAll(this);
	}
	if (UChopItCycleStateMachineComponent* Cycle = GetOwner() ? GetOwner()->FindComponentByClass<UChopItCycleStateMachineComponent>() : nullptr)
	{
		Cycle->OnPhaseChanged.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void UChopItVisualStyleComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (TransitionDuration > 0.0f && TransitionElapsed < TransitionDuration)
	{
		TransitionElapsed = FMath::Min(TransitionElapsed + DeltaTime, TransitionDuration);
		const float Alpha = FMath::SmoothStep(0.0f, 1.0f, TransitionElapsed / TransitionDuration);
		CurrentValues = LerpValues(StartValues, TargetValues, Alpha);
	}
	else
	{
		CurrentValues = TargetValues;
	}

	if (IntensityTransitionDuration > 0.0f && IntensityTransitionElapsed < IntensityTransitionDuration)
	{
		IntensityTransitionElapsed = FMath::Min(IntensityTransitionElapsed + DeltaTime, IntensityTransitionDuration);
		const float Alpha = FMath::SmoothStep(0.0f, 1.0f, IntensityTransitionElapsed / IntensityTransitionDuration);
		CurrentGlobalIntensity = FMath::Lerp(StartGlobalIntensity, TargetGlobalIntensity, Alpha);
	}
	else
	{
		CurrentGlobalIntensity = TargetGlobalIntensity;
	}

	DamagePulse = FMath::FInterpConstantTo(DamagePulse, 0.0f, DeltaTime, 1.0f / FMath::Max(0.05f, DamagePulseDuration));
	DangerIntensity = FMath::FInterpTo(DangerIntensity, TargetDangerIntensity, DeltaTime, 2.8f);

	OutlineRefreshElapsed += DeltaTime;
	if (OutlineRefreshElapsed >= 0.75f)
	{
		OutlineRefreshElapsed = 0.0f;
		RefreshOutlinedActors();
		BindPlayerDamage();
	}
	ApplyCurrentValues();

	if (!CaptureFilename.IsEmpty())
	{
		CaptureElapsed += DeltaTime;
		if (!bCaptureRequested && CaptureElapsed >= 3.0f)
		{
			FScreenshotRequest::RequestScreenshot(CaptureFilename, false, false, false, FIntRect(), true);
			bCaptureRequested = true;
		}
		else if (bCaptureRequested && CaptureElapsed >= 4.5f)
		{
			FPlatformMisc::RequestExit(false);
		}
	}
}

void UChopItVisualStyleComponent::ApplyPreset(const EChopItVisualPreset NewPreset, const float TransitionSeconds)
{
	if (const UChopItVisualStylePreset* Preset = ResolvePreset(NewPreset))
	{
		BasePreset = NewPreset;
		StartValues = CurrentValues;
		TargetValues = ValuesFromPreset(Preset);
		TransitionElapsed = 0.0f;
		TransitionDuration = FMath::Max(0.0f, TransitionSeconds);
		if (TransitionDuration <= 0.0f) CurrentValues = TargetValues;
	}
}

void UChopItVisualStyleComponent::SetGlobalIntensity(const float NewIntensity, const float TransitionSeconds)
{
	StartGlobalIntensity = CurrentGlobalIntensity;
	TargetGlobalIntensity = FMath::Clamp(NewIntensity, 0.0f, 1.0f);
	IntensityTransitionElapsed = 0.0f;
	IntensityTransitionDuration = FMath::Max(0.0f, TransitionSeconds);
}

void UChopItVisualStyleComponent::TriggerDamageDegradation(const float Strength, const float Duration)
{
	DamagePulse = FMath::Max(DamagePulse, FMath::Clamp(Strength, 0.0f, 2.0f));
	DamagePulseDuration = FMath::Max(0.05f, Duration);
}

void UChopItVisualStyleComponent::SetDangerDegradation(const float NewIntensity, const float)
{
	TargetDangerIntensity = FMath::Clamp(NewIntensity, 0.0f, 1.0f);
}

void UChopItVisualStyleComponent::RestoreBasePreset(const float TransitionSeconds)
{
	ApplyPreset(BasePreset, TransitionSeconds);
	TargetDangerIntensity = 0.0f;
	DamagePulse = 0.0f;
}

void UChopItVisualStyleComponent::SetActorOutline(AActor* Actor, const bool bEnabled, const int32 StencilValue)
{
	if (!IsValid(Actor)) return;
	const TCHAR* OverlayPath = StencilValue <= 1
		? ChopItVisualStyle::PlayerOutlineMaterialPath
		: (StencilValue == 2 ? ChopItVisualStyle::EnemyOutlineMaterialPath : ChopItVisualStyle::InteractableOutlineMaterialPath);
	UMaterialInterface* OverlayMaterial = bEnabled ? LoadObject<UMaterialInterface>(nullptr, OverlayPath) : nullptr;
	TArray<UMeshComponent*> Meshes;
	Actor->GetComponents<UMeshComponent>(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh)) continue;
		Mesh->SetRenderCustomDepth(bEnabled);
		Mesh->SetCustomDepthStencilWriteMask(ERendererStencilMask::ERSM_Default);
		Mesh->SetCustomDepthStencilValue(FMath::Clamp(StencilValue, 0, 255));
		// The inverted-hull overlay is a deterministic visible fallback for render paths where
		// a post-process material cannot sample CustomStencil. The stencil remains
		// enabled for the external silhouette when that buffer is available.
		Mesh->SetOverlayMaterial(OverlayMaterial);
	}
}

const UChopItVisualStylePreset* UChopItVisualStyleComponent::ResolvePreset(const EChopItVisualPreset Preset) const
{
	const TCHAR* Path = ChopItVisualStyle::HybridPresetPath;
	switch (Preset)
	{
	case EChopItVisualPreset::Megabonk: Path = ChopItVisualStyle::MegabonkPresetPath; break;
	case EChopItVisualPreset::MachineParty: Path = ChopItVisualStyle::MachinePartyPresetPath; break;
	default: break;
	}
	return LoadObject<UChopItVisualStylePreset>(nullptr, Path);
}

UChopItVisualStyleComponent::FRuntimeValues UChopItVisualStyleComponent::ValuesFromPreset(const UChopItVisualStylePreset* Preset)
{
	FRuntimeValues Values;
	if (!Preset) return Values;
	Values.ColorSteps = Preset->ColorSteps;
	Values.Contrast = Preset->Contrast;
	Values.Saturation = Preset->Saturation;
	Values.ExposureBias = Preset->ExposureBias;
	Values.ColorGain = Preset->ColorGain;
	Values.AmbientTint = Preset->AmbientTint;
	Values.AmbientTintStrength = Preset->AmbientTintStrength;
	Values.Pixelation = Preset->bPixelation ? 1.0f : 0.0f;
	Values.VirtualResolutionHeight = Preset->VirtualResolutionHeight;
	Values.Dithering = Preset->bDithering ? 1.0f : 0.0f;
	Values.DitherStrength = Preset->DitherStrength;
	Values.FilmGrain = Preset->bFilmGrain ? 1.0f : 0.0f;
	Values.GrainIntensity = Preset->GrainIntensity;
	Values.Vignette = Preset->bVignette ? 1.0f : 0.0f;
	Values.VignetteIntensity = Preset->VignetteIntensity;
	Values.ChromaticAberration = Preset->bChromaticAberration ? 1.0f : 0.0f;
	Values.ChromaticAberrationPixels = Preset->ChromaticAberrationPixels;
	Values.Scanlines = Preset->bScanlines ? 1.0f : 0.0f;
	Values.ScanlineIntensity = Preset->ScanlineIntensity;
	Values.ScreenDistortion = Preset->bScreenDistortion ? 1.0f : 0.0f;
	Values.DistortionIntensity = Preset->DistortionIntensity;
	Values.DepthFog = Preset->bDepthFog ? 1.0f : 0.0f;
	Values.DepthFogDensity = Preset->DepthFogDensity;
	Values.Outlines = Preset->bOutlines ? 1.0f : 0.0f;
	Values.OutlineThickness = Preset->OutlineThickness;
	Values.GlobalOutline = Preset->bGlobalOutline ? 1.0f : 0.0f;
	Values.GlobalOutlineColor = Preset->GlobalOutlineColor;
	Values.GlobalOutlineThickness = Preset->GlobalOutlineThickness;
	Values.GlobalOutlineDepthThreshold = Preset->GlobalOutlineDepthThreshold;
	Values.GlobalOutlineNormalThreshold = Preset->GlobalOutlineNormalThreshold;
	Values.GlobalOutlineStrength = Preset->GlobalOutlineStrength;
	Values.PlayerOutlineColor = Preset->PlayerOutlineColor;
	Values.EnemyOutlineColor = Preset->EnemyOutlineColor;
	Values.InteractableOutlineColor = Preset->InteractableOutlineColor;
	Values.FogColor = Preset->FogColor;
	Values.FogDensity = Preset->FogDensity;
	Values.FogStartDistance = Preset->FogStartDistance;
	Values.FogMaxOpacity = Preset->FogMaxOpacity;
	Values.FogHeightFalloff = Preset->FogHeightFalloff;
	Values.BloomIntensity = Preset->BloomIntensity;
	return Values;
}

UChopItVisualStyleComponent::FRuntimeValues UChopItVisualStyleComponent::LerpValues(
	const FRuntimeValues& A,
	const FRuntimeValues& B,
	const float Alpha)
{
	FRuntimeValues V;
#define CHOPIT_LERP_FIELD(Name) V.Name = FMath::Lerp(A.Name, B.Name, Alpha)
	CHOPIT_LERP_FIELD(ColorSteps);
	CHOPIT_LERP_FIELD(Contrast);
	CHOPIT_LERP_FIELD(Saturation);
	CHOPIT_LERP_FIELD(ExposureBias);
	CHOPIT_LERP_FIELD(ColorGain);
	V.AmbientTint = FLinearColor::LerpUsingHSV(A.AmbientTint, B.AmbientTint, Alpha);
	CHOPIT_LERP_FIELD(AmbientTintStrength);
	CHOPIT_LERP_FIELD(Pixelation);
	CHOPIT_LERP_FIELD(VirtualResolutionHeight);
	CHOPIT_LERP_FIELD(Dithering);
	CHOPIT_LERP_FIELD(DitherStrength);
	CHOPIT_LERP_FIELD(FilmGrain);
	CHOPIT_LERP_FIELD(GrainIntensity);
	CHOPIT_LERP_FIELD(Vignette);
	CHOPIT_LERP_FIELD(VignetteIntensity);
	CHOPIT_LERP_FIELD(ChromaticAberration);
	CHOPIT_LERP_FIELD(ChromaticAberrationPixels);
	CHOPIT_LERP_FIELD(Scanlines);
	CHOPIT_LERP_FIELD(ScanlineIntensity);
	CHOPIT_LERP_FIELD(ScreenDistortion);
	CHOPIT_LERP_FIELD(DistortionIntensity);
	CHOPIT_LERP_FIELD(DepthFog);
	CHOPIT_LERP_FIELD(DepthFogDensity);
	CHOPIT_LERP_FIELD(Outlines);
	CHOPIT_LERP_FIELD(OutlineThickness);
	CHOPIT_LERP_FIELD(GlobalOutline);
	V.GlobalOutlineColor = FLinearColor::LerpUsingHSV(A.GlobalOutlineColor, B.GlobalOutlineColor, Alpha);
	CHOPIT_LERP_FIELD(GlobalOutlineThickness);
	CHOPIT_LERP_FIELD(GlobalOutlineDepthThreshold);
	CHOPIT_LERP_FIELD(GlobalOutlineNormalThreshold);
	CHOPIT_LERP_FIELD(GlobalOutlineStrength);
	V.PlayerOutlineColor = FLinearColor::LerpUsingHSV(A.PlayerOutlineColor, B.PlayerOutlineColor, Alpha);
	V.EnemyOutlineColor = FLinearColor::LerpUsingHSV(A.EnemyOutlineColor, B.EnemyOutlineColor, Alpha);
	V.InteractableOutlineColor = FLinearColor::LerpUsingHSV(A.InteractableOutlineColor, B.InteractableOutlineColor, Alpha);
	V.FogColor = FLinearColor::LerpUsingHSV(A.FogColor, B.FogColor, Alpha);
	CHOPIT_LERP_FIELD(FogDensity);
	CHOPIT_LERP_FIELD(FogStartDistance);
	CHOPIT_LERP_FIELD(FogMaxOpacity);
	CHOPIT_LERP_FIELD(FogHeightFalloff);
	CHOPIT_LERP_FIELD(BloomIntensity);
#undef CHOPIT_LERP_FIELD
	return V;
}

void UChopItVisualStyleComponent::ConfigurePostProcess()
{
	if (!GetOwner()) return;
	GlobalOutlineMaterial = LoadObject<UMaterialInterface>(nullptr, ChopItVisualStyle::GlobalOutlineMaterialPath);

	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("ChopItVisualStylePreview")))
		{
			PostProcessVolume = *It;
			break;
		}
	}

	FPostProcessSettings* SettingsPtr = nullptr;
	if (PostProcessVolume)
	{
		PostProcessVolume->bEnabled = true;
		PostProcessVolume->bUnbound = true;
		PostProcessVolume->Priority = 50.0f;
		PostProcessVolume->BlendWeight = 1.0f;
		PostProcessVolume->Settings.WeightedBlendables.Array.Reset();
		SettingsPtr = &PostProcessVolume->Settings;
	}
	else
	{
		PostProcessComponent = NewObject<UPostProcessComponent>(GetOwner(), TEXT("ChopItVisualStylePostProcess"));
		GetOwner()->AddInstanceComponent(PostProcessComponent);
		PostProcessComponent->bUnbound = true;
		PostProcessComponent->BlendWeight = 1.0f;
		PostProcessComponent->Priority = 50.0f;
		SettingsPtr = &PostProcessComponent->Settings;
	}

	FPostProcessSettings& Settings = *SettingsPtr;
	Settings.bOverride_ColorSaturation = true;
	Settings.ColorSaturation = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	Settings.bOverride_ColorContrast = true;
	Settings.ColorContrast = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	Settings.bOverride_VignetteIntensity = true;
	Settings.VignetteIntensity = 0.0f;
	Settings.bOverride_BloomIntensity = true;
	Settings.BloomIntensity = 0.18f;
	Settings.bOverride_AutoExposureMethod = false;
	Settings.bOverride_AutoExposureBias = true;
	Settings.AutoExposureBias = -0.65f;
	Settings.bOverride_MotionBlurAmount = true;
	Settings.MotionBlurAmount = 0.0f;
	Settings.bOverride_SceneFringeIntensity = true;
	Settings.SceneFringeIntensity = 0.0f;
	Settings.bOverride_AmbientOcclusionIntensity = true;
	Settings.AmbientOcclusionIntensity = 0.35f;
	if (PostProcessComponent)
	{
		// Register only after the blendable and overrides are present. Registering first
		// leaves the initial render proxy with an empty blendable list on some RHIs.
		PostProcessComponent->RegisterComponent();
	}
}

void UChopItVisualStyleComponent::ApplyCurrentValues()
{
	const float Degradation = FMath::Clamp(DamagePulse * 0.8f + DangerIntensity * 0.45f, 0.0f, 1.5f);
	if (PostProcessMaterial)
	{
		auto SetScalar = [this](const TCHAR* Name, const float Value)
		{
			PostProcessMaterial->SetScalarParameterValue(Name, Value);
		};
		auto SetVector = [this](const TCHAR* Name, const FLinearColor& Value)
		{
			PostProcessMaterial->SetVectorParameterValue(Name, Value);
		};

	SetScalar(TEXT("GlobalIntensity"), CurrentGlobalIntensity);
	SetScalar(TEXT("ColorSteps"), CurrentValues.ColorSteps);
	SetScalar(TEXT("Contrast"), CurrentValues.Contrast);
	SetScalar(TEXT("Saturation"), CurrentValues.Saturation);
	SetScalar(TEXT("ExposureBias"), CurrentValues.ExposureBias);
	SetScalar(TEXT("ColorGain"), CurrentValues.ColorGain);
	SetVector(TEXT("AmbientTint"), CurrentValues.AmbientTint);
	SetScalar(TEXT("AmbientTintStrength"), CurrentValues.AmbientTintStrength + DangerIntensity * 0.08f);
	SetScalar(TEXT("PixelationEnabled"), CurrentValues.Pixelation);
	SetScalar(TEXT("VirtualResolutionHeight"), CurrentValues.VirtualResolutionHeight);
	SetScalar(TEXT("DitheringEnabled"), CurrentValues.Dithering);
	SetScalar(TEXT("DitherStrength"), CurrentValues.DitherStrength);
	SetScalar(TEXT("GrainEnabled"), FMath::Max(CurrentValues.FilmGrain, Degradation > 0.001f ? 1.0f : 0.0f));
	SetScalar(TEXT("GrainIntensity"), CurrentValues.GrainIntensity + Degradation * 0.10f);
	SetScalar(TEXT("VignetteEnabled"), FMath::Max(CurrentValues.Vignette, Degradation > 0.001f ? 1.0f : 0.0f));
	SetScalar(TEXT("VignetteIntensity"), CurrentValues.VignetteIntensity + Degradation * 0.18f);
	SetScalar(TEXT("ChromaticAberrationEnabled"), CurrentValues.ChromaticAberration);
	SetScalar(TEXT("ChromaticAberrationPixels"), CurrentValues.ChromaticAberrationPixels + DamagePulse * 0.65f);
	SetScalar(TEXT("ScanlinesEnabled"), CurrentValues.Scanlines);
	SetScalar(TEXT("ScanlineIntensity"), CurrentValues.ScanlineIntensity);
	SetScalar(TEXT("DistortionEnabled"), FMath::Max(CurrentValues.ScreenDistortion, Degradation > 0.001f ? 1.0f : 0.0f));
	SetScalar(TEXT("DistortionIntensity"), CurrentValues.DistortionIntensity + Degradation * 0.012f);
	SetScalar(TEXT("DepthFogEnabled"), CurrentValues.DepthFog);
	SetScalar(TEXT("DepthFogDensity"), CurrentValues.DepthFogDensity);
	SetVector(TEXT("DepthFogColor"), CurrentValues.FogColor);
	SetScalar(TEXT("OutlineEnabled"), CurrentValues.Outlines);
	SetScalar(TEXT("OutlineThickness"), CurrentValues.OutlineThickness);
	SetScalar(TEXT("GlobalOutlineEnabled"), CurrentValues.GlobalOutline);
	SetVector(TEXT("GlobalOutlineColor"), CurrentValues.GlobalOutlineColor);
	SetScalar(TEXT("GlobalOutlineThickness"), CurrentValues.GlobalOutlineThickness);
	SetScalar(TEXT("GlobalOutlineDepthThreshold"), CurrentValues.GlobalOutlineDepthThreshold);
	SetScalar(TEXT("GlobalOutlineNormalThreshold"), CurrentValues.GlobalOutlineNormalThreshold);
	SetScalar(TEXT("GlobalOutlineStrength"), CurrentValues.GlobalOutlineStrength);
	SetVector(TEXT("PlayerOutlineColor"), CurrentValues.PlayerOutlineColor);
	SetVector(TEXT("EnemyOutlineColor"), CurrentValues.EnemyOutlineColor);
	SetVector(TEXT("InteractableOutlineColor"), CurrentValues.InteractableOutlineColor);
		SetScalar(TEXT("StyleTime"), GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);
	}

	FPostProcessSettings* ActiveSettings = PostProcessVolume
		? &PostProcessVolume->Settings
		: (PostProcessComponent ? &PostProcessComponent->Settings : nullptr);
	if (ActiveSettings)
	{
		ActiveSettings->BloomIntensity = CurrentValues.BloomIntensity;
		ActiveSettings->AutoExposureBias = CurrentValues.ExposureBias;
		if (PostProcessComponent) PostProcessComponent->MarkRenderStateDirty();
	}
	if (HeightFogComponent)
	{
		HeightFogComponent->SetFogInscatteringColor(CurrentValues.FogColor);
		HeightFogComponent->SetFogDensity(CurrentValues.FogDensity);
		HeightFogComponent->SetFogMaxOpacity(CurrentValues.FogMaxOpacity);
		HeightFogComponent->SetStartDistance(CurrentValues.FogStartDistance);
		HeightFogComponent->SetFogHeightFalloff(CurrentValues.FogHeightFalloff);
	}
}

void UChopItVisualStyleComponent::RefreshOutlinedActors()
{
	if (!GetWorld()) return;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It) ConfigureOutlineForActor(*It);
}

void UChopItVisualStyleComponent::HandleActorSpawned(AActor* Actor)
{
	ConfigureOutlineForActor(Actor);
}

void UChopItVisualStyleComponent::ConfigureOutlineForActor(AActor* Actor)
{
	if (!IsValid(Actor)) return;
	if (Actor->IsA<AChopItCharacter>()) SetActorOutline(Actor, true, 1);
	else if (Actor->IsA<AChopItEnemyCharacter>()) SetActorOutline(Actor, true, 2);
	else if (Actor->GetClass()->ImplementsInterface(UChopItInteractable::StaticClass())) SetActorOutline(Actor, true, 3);
	else
	{
		TArray<UMeshComponent*> Meshes;
		Actor->GetComponents<UMeshComponent>(Meshes);
		for (UMeshComponent* Mesh : Meshes)
		{
			if (IsValid(Mesh) && Mesh->IsVisible())
			{
				Mesh->SetOverlayMaterial(CurrentValues.GlobalOutline > 0.001f && CurrentGlobalIntensity > 0.001f
					? GlobalOutlineMaterial.Get()
					: nullptr);
			}
		}
	}
}

void UChopItVisualStyleComponent::BindPlayerDamage()
{
	if (BoundPlayerHealth.IsValid() || !GetWorld()) return;
	if (APawn* Pawn = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr)
	{
		if (UChopItHealthComponent* Health = Pawn->FindComponentByClass<UChopItHealthComponent>())
		{
			BoundPlayerHealth = Health;
			Health->OnDamageReceived.AddUObject(this, &UChopItVisualStyleComponent::HandlePlayerDamaged);
		}
	}
}

void UChopItVisualStyleComponent::HandlePlayerDamaged(
	const float Damage,
	const bool bCritical,
	AActor*,
	const FVector&)
{
	TriggerDamageDegradation(FMath::Clamp(Damage / 35.0f + (bCritical ? 0.35f : 0.0f), 0.25f, 1.35f));
}

void UChopItVisualStyleComponent::HandleCyclePhaseChanged(
	const EChopItCyclePhase NewPhase,
	const EChopItCyclePhase,
	const int32)
{
	switch (NewPhase)
	{
	case EChopItCyclePhase::Night: SetDangerDegradation(0.45f); break;
	case EChopItCyclePhase::Elite: SetDangerDegradation(0.9f); break;
	case EChopItCyclePhase::Death: SetDangerDegradation(1.0f); break;
	default: SetDangerDegradation(0.0f); break;
	}
}
