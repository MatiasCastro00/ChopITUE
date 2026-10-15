#include "Phone/ChopItPhoneCallComponent.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Phone/ChopItPhoneCallConfig.h"
#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneStateComponent.h"
#include "Sound/SoundAttenuation.h"

UChopItPhoneCallComponent::UChopItPhoneCallComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CallConfig = TSoftObjectPtr<UChopItPhoneCallConfig>(FSoftObjectPath(TEXT("/Game/NewChopIt/Art/Models/Phone/DA_PhoneCall.DA_PhoneCall")));
}

void UChopItPhoneCallComponent::BeginPlay()
{
	Super::BeginPlay();
	PhoneState = GetOwner() ? GetOwner()->FindComponentByClass<UChopItPhoneStateComponent>() : nullptr;
	ActiveConfig = CallConfig.LoadSynchronous();
	if (!PhoneState || !ActiveConfig || !ActiveConfig->IsValidConfig() || !GetOwner()->GetRootComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("Phone call disabled on %s: missing state, root or valid DA_PhoneCall"), *GetNameSafe(GetOwner()));
		return;
	}
	PhoneState->OnStateChanged.AddDynamic(this, &UChopItPhoneCallComponent::HandleStateChanged);
	const auto CreateRingAudio = [this](const FName Name)
	{
		UAudioComponent* Audio = NewObject<UAudioComponent>(GetOwner(), Name);
		GetOwner()->AddInstanceComponent(Audio);
		Audio->SetupAttachment(GetOwner()->GetRootComponent());
		Audio->SetRelativeLocation(FVector(0, 0, 50));
		Audio->bAutoActivate = false;
		Audio->bAllowSpatialization = true;
		Audio->bIsUISound = false;
		Audio->bOverrideAttenuation = true;
		FSoundAttenuationSettings Attenuation = ActiveConfig->AttenuationAsset
			? ActiveConfig->AttenuationAsset->Attenuation : FSoundAttenuationSettings();
		Attenuation.bAttenuate = true;
		Attenuation.bSpatialize = true;
		Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
		Attenuation.AttenuationShape = EAttenuationShape::Sphere;
		const float AudibleDistance = GetAudibleDistance();
		const float FullVolumeRadius = FMath::Min(ActiveConfig->FullVolumeRadius, AudibleDistance - 1.0f);
		Attenuation.AttenuationShapeExtents = FVector(FullVolumeRadius, 0, 0);
		Attenuation.FalloffDistance = AudibleDistance - FullVolumeRadius;
		Audio->SetAttenuationOverrides(Attenuation);
		Audio->SetVolumeMultiplier(ActiveConfig->Volume);
		Audio->SetSound(ActiveConfig->RingSound);
		Audio->RegisterComponent();
		return Audio;
	};
	RingAudio = CreateRingAudio(TEXT("PhoneRingAudio"));
	SecondaryRingAudio = CreateRingAudio(TEXT("PhoneRingAudioSecondary"));
	Random.Initialize(FMath::Rand());
	if (PhoneState->GetState() == EChopItPhoneState::Available) ScheduleNextCall();
}

float UChopItPhoneCallComponent::GetAudibleDistance() const
{
	if (const AChopItMysteryPhone* Phone = Cast<AChopItMysteryPhone>(GetOwner()))
		return Phone->GetHearingRange();
	return ActiveConfig ? ActiveConfig->AudibleDistance : 0.0f;
}

void UChopItPhoneCallComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(IntervalTimer);
		GetWorld()->GetTimerManager().ClearTimer(DurationTimer);
		GetWorld()->GetTimerManager().ClearTimer(PairTimer);
		GetWorld()->GetTimerManager().ClearTimer(SecondStrikeTimer);
	}
	if (PhoneState) PhoneState->OnStateChanged.RemoveDynamic(this, &UChopItPhoneCallComponent::HandleStateChanged);
	StopAudio();
	Super::EndPlay(EndPlayReason);
}

bool UChopItPhoneCallComponent::IsCallScheduled() const
{
	return GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(IntervalTimer);
}

void UChopItPhoneCallComponent::ScheduleNextCall()
{
	if (!GetWorld() || !ActiveConfig || !PhoneState || PhoneState->GetState() != EChopItPhoneState::Available) return;
	const float Delay = bHasRung
		? FMath::Max(0.1f, Random.FRandRange(ActiveConfig->MinIntervalSeconds,
			ActiveConfig->MaxIntervalSeconds) - ActiveConfig->RingDurationSeconds)
		: ActiveConfig->FirstCallDelaySeconds;
	GetWorld()->GetTimerManager().SetTimer(IntervalTimer, this, &UChopItPhoneCallComponent::StartScheduledCall, Delay, false);
	UE_LOG(LogTemp, Display, TEXT("Phone call scheduled on %s in %.1f seconds."), *GetNameSafe(GetOwner()), Delay);
}

void UChopItPhoneCallComponent::StartScheduledCall()
{
	StartCall();
}

bool UChopItPhoneCallComponent::StartCall()
{
	if (!GetWorld() || !ActiveConfig || !RingAudio || !PhoneState ||
		PhoneState->GetState() != EChopItPhoneState::Available || !PhoneState->TryTransition(EChopItPhoneState::Ringing)) return false;
	GetWorld()->GetTimerManager().ClearTimer(IntervalTimer);
	bHasRung = true;
	GetWorld()->GetTimerManager().SetTimer(DurationTimer, this, &UChopItPhoneCallComponent::EndCall,
		ActiveConfig->RingDurationSeconds, false);
	OnRingStarted.Broadcast();
	NextBeatIndex = 0;
	NextPairIndex = 0;
	EmitPair();
	if (ActiveConfig->RingPairsPerCall > 1)
		GetWorld()->GetTimerManager().SetTimer(PairTimer, this, &UChopItPhoneCallComponent::EmitPair,
			ActiveConfig->RingPairSpacingSeconds, true);
	UE_LOG(LogTemp, Display, TEXT("Phone ringing on %s: sound=%s playing=%s."),
		*GetNameSafe(GetOwner()), *GetNameSafe(RingAudio->Sound), RingAudio->IsPlaying() ? TEXT("true") : TEXT("false"));
	return true;
}

void UChopItPhoneCallComponent::EmitPair()
{
	if (!ActiveConfig || !PhoneState || PhoneState->GetState() != EChopItPhoneState::Ringing) return;
	++NextPairIndex;
	EmitNextBeat();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(SecondStrikeTimer, this,
			&UChopItPhoneCallComponent::EmitNextBeat, ActiveConfig->RingStrikeSpacingSeconds, false);
		if (NextPairIndex >= ActiveConfig->RingPairsPerCall)
			GetWorld()->GetTimerManager().ClearTimer(PairTimer);
	}
}

void UChopItPhoneCallComponent::EmitNextBeat()
{
	if (!ActiveConfig || !PhoneState || PhoneState->GetState() != EChopItPhoneState::Ringing) return;
	UAudioComponent* Audio = NextBeatIndex % 2 == 0 ? RingAudio : SecondaryRingAudio;
	if (Audio) Audio->Play();
	OnRingBeat.Broadcast(NextBeatIndex++);
}

void UChopItPhoneCallComponent::EndCall()
{
	if (PhoneState && PhoneState->GetState() == EChopItPhoneState::Ringing)
	{
		PhoneState->TryTransition(EChopItPhoneState::Available);
	}
}

void UChopItPhoneCallComponent::StopAudio()
{
	if (RingAudio && RingAudio->IsPlaying()) RingAudio->Stop();
	if (SecondaryRingAudio && SecondaryRingAudio->IsPlaying()) SecondaryRingAudio->Stop();
}

void UChopItPhoneCallComponent::HandleStateChanged(EChopItPhoneState Previous, EChopItPhoneState Current)
{
	if (Current == EChopItPhoneState::Ringing) return;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PairTimer);
		GetWorld()->GetTimerManager().ClearTimer(SecondStrikeTimer);
	}
	if (Current == EChopItPhoneState::Available && Previous == EChopItPhoneState::Ringing)
	{
		StopAudio();
		OnRingStopped.Broadcast();
		ScheduleNextCall();
		return;
	}
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(IntervalTimer);
		GetWorld()->GetTimerManager().ClearTimer(DurationTimer);
	}
	if (Previous == EChopItPhoneState::Ringing)
	{
		StopAudio();
		OnRingStopped.Broadcast();
	}
}
