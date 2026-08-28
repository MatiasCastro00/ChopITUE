#include "Feedback/ChopItFeedbackAudio.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{
	void PlaySynthTone(UObject* WorldContextObject, const FVector& Location, const float Frequency, const float Duration, const float Volume, const float NoiseAmount)
	{
		if (!WorldContextObject || Volume <= 0.0f)
		{
			return;
		}
		constexpr int32 SampleRate = 22050;
		const int32 SampleCount = FMath::Max(1, FMath::RoundToInt(Duration * SampleRate));
		TArray<int16> Samples;
		Samples.SetNumUninitialized(SampleCount);
		for (int32 Index = 0; Index < SampleCount; ++Index)
		{
			const float Time = static_cast<float>(Index) / static_cast<float>(SampleRate);
			const float Envelope = FMath::Square(FMath::Clamp(1.0f - Time / Duration, 0.0f, 1.0f));
			const float Sine = FMath::Sin(2.0f * PI * Frequency * Time);
			const float Noise = FMath::FRandRange(-1.0f, 1.0f) * NoiseAmount;
			Samples[Index] = static_cast<int16>(FMath::Clamp((Sine + Noise) * Envelope, -1.0f, 1.0f) * 12000.0f);
		}
		USoundWaveProcedural* Sound = NewObject<USoundWaveProcedural>(GetTransientPackage(), NAME_None, RF_Transient);
		Sound->SetSampleRate(SampleRate);
		Sound->NumChannels = 1;
		Sound->Duration = Duration;
		Sound->bLooping = false;
		Sound->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
		UGameplayStatics::PlaySoundAtLocation(WorldContextObject, Sound, Location, Volume, 1.0f);
	}
}

void ChopItFeedbackAudio::PlayWoodImpact(UObject* WorldContextObject, const FVector& Location, const bool bCritical, const float Volume)
{
	PlaySynthTone(WorldContextObject, Location, bCritical ? 215.0f : 145.0f, bCritical ? 0.14f : 0.10f, Volume, 0.48f);
}

void ChopItFeedbackAudio::PlayAxeSwing(UObject* WorldContextObject, const FVector& Location, const float Volume)
{
	PlaySynthTone(WorldContextObject, Location, 95.0f, 0.055f, Volume * 0.35f, 0.18f);
}

void ChopItFeedbackAudio::PlayPhaseSting(UObject* WorldContextObject, const FVector& Location, const int32 PhaseIndex, const float Volume)
{
	const float Frequency = PhaseIndex == 2 ? 175.0f : PhaseIndex == 3 ? 88.0f : PhaseIndex == 4 ? 245.0f : 320.0f;
	PlaySynthTone(WorldContextObject, Location, Frequency, 0.24f, Volume * 0.55f, 0.08f);
}
