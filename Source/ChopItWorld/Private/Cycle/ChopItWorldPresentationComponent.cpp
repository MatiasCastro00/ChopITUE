#include "Cycle/ChopItWorldPresentationComponent.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/AudioComponent.h"
#include "ChopItDeveloperSettings.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "Sound/SoundBase.h"
#include "Feedback/ChopItFeedbackAudio.h"

UChopItWorldPresentationComponent::UChopItWorldPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UChopItWorldPresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	MusicA = NewObject<UAudioComponent>(GetOwner(), TEXT("ChopItDayNightMusicA"));
	MusicB = NewObject<UAudioComponent>(GetOwner(), TEXT("ChopItDayNightMusicB"));
	for (UAudioComponent* Music : {MusicA.Get(), MusicB.Get()})
	{
		if (Music)
		{
			Music->bAutoActivate = false;
			Music->bIsUISound = true;
			Music->RegisterComponent();
		}
	}
	if (UChopItCycleStateMachineComponent* Cycle = GetOwner()->FindComponentByClass<UChopItCycleStateMachineComponent>())
	{
		Cycle->OnPhaseChanged.AddUniqueDynamic(this, &UChopItWorldPresentationComponent::HandlePhaseChanged);
		HandlePhaseChanged(Cycle->GetCurrentPhase(), EChopItCyclePhase::Bootstrap, Cycle->GetPhaseGeneration());
	}
}

void UChopItWorldPresentationComponent::HandlePhaseChanged(
	const EChopItCyclePhase NewPhase,
	const EChopItCyclePhase,
	const int32)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	float Intensity = 5.0f;
	FLinearColor Color(1.0f, 0.88f, 0.70f);
	switch (NewPhase)
	{
	case EChopItCyclePhase::Dusk: Intensity = 2.0f; Color = FLinearColor(1.0f, 0.28f, 0.06f); break;
	case EChopItCyclePhase::Night: Intensity = 0.35f; Color = FLinearColor(0.12f, 0.22f, 1.0f); break;
	case EChopItCyclePhase::Elite: Intensity = 0.7f; Color = FLinearColor(1.0f, 0.02f, 0.02f); break;
	case EChopItCyclePhase::Resolution: Intensity = 3.0f; Color = FLinearColor(0.25f, 1.0f, 0.35f); break;
	case EChopItCyclePhase::Death: Intensity = 0.1f; Color = FLinearColor(0.5f, 0.0f, 0.0f); break;
	default: break;
	}

	if (!DirectionalLight.IsValid())
	{
		for (TActorIterator<ADirectionalLight> It(World); It; ++It)
		{
			DirectionalLight = Cast<UDirectionalLightComponent>(It->GetLightComponent());
			break;
		}
	}
	if (!DirectionalLight.IsValid())
	{
		return;
	}

	TransitionStartIntensity = DirectionalLight->Intensity;
	TransitionStartColor = DirectionalLight->GetLightColor();
	TransitionTargetIntensity = Intensity;
	TransitionTargetColor = Color;
	TransitionElapsed = 0.0f;
	SetComponentTickEnabled(true);
	TransitionMusic(NewPhase);
	if (const UChopItDeveloperSettings* Settings = GetDefault<UChopItDeveloperSettings>(); Settings && Settings->bEnableImpactSounds)
	{
		ChopItFeedbackAudio::PlayPhaseSting(this, GetOwner()->GetActorLocation(), static_cast<int32>(NewPhase), Settings->EffectsVolume);
	}
}

void UChopItWorldPresentationComponent::TransitionMusic(const EChopItCyclePhase NewPhase)
{
	USoundBase* NextSound = ResolveMusic(NewPhase);
	if (!NextSound || !MusicA || !MusicB)
	{
		return;
	}
	UAudioComponent* Previous = bMusicAActive ? MusicA : MusicB;
	UAudioComponent* Next = bMusicAActive ? MusicB : MusicA;
	if (Previous && Previous->Sound == NextSound)
	{
		return;
	}
	if (Previous && Previous->IsPlaying())
	{
		Previous->FadeOut(MusicCrossfadeDuration, 0.0f);
	}
	Next->SetSound(NextSound);
	Next->FadeIn(MusicCrossfadeDuration, 1.0f);
	bMusicAActive = !bMusicAActive;
}

USoundBase* UChopItWorldPresentationComponent::ResolveMusic(const EChopItCyclePhase Phase) const
{
	switch (Phase)
	{
	case EChopItCyclePhase::Dusk: return DuskMusic.LoadSynchronous();
	case EChopItCyclePhase::Night: return NightMusic.LoadSynchronous();
	case EChopItCyclePhase::Elite: return EliteMusic.LoadSynchronous();
	default: return DayMusic.LoadSynchronous();
	}
}

void UChopItWorldPresentationComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!DirectionalLight.IsValid())
	{
		SetComponentTickEnabled(false);
		return;
	}

	TransitionElapsed += DeltaTime;
	const float LinearAlpha = FMath::Clamp(TransitionElapsed / LightTransitionDuration, 0.0f, 1.0f);
	const float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, LinearAlpha);
	DirectionalLight->SetIntensity(FMath::Lerp(TransitionStartIntensity, TransitionTargetIntensity, SmoothAlpha));
	DirectionalLight->SetLightColor(FLinearColor::LerpUsingHSV(TransitionStartColor, TransitionTargetColor, SmoothAlpha));
	if (LinearAlpha >= 1.0f)
	{
		SetComponentTickEnabled(false);
	}
}
