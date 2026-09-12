#pragma once
#include "Engine/DataAsset.h"
#include "Sound/SoundBase.h"
#include "Curves/CurveFloat.h"
#include "ChopItUIJuiceProfile.generated.h"

UCLASS(BlueprintType)
class CHOPIT_API UChopItUIJuiceProfile : public UDataAsset
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clock") float ClockIconSwingDegrees = 20.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clock", meta=(ClampMin="0.1")) float ClockIconSwingPeriod = 2.4f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clock", meta=(ClampMin="0.01")) float ClockIconFadeDuration = .45f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clock", meta=(ClampMin="0",ClampMax="0.5")) float ClockIconUrgentPulse = .2f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion") bool bReducedMotion = false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion") float PunchScale = 1.25f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion") float WobbleDegrees = 12.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion") float PunchDuration = .45f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion") float MissionDuration = .6f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion") TObjectPtr<UCurveFloat> Envelope;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Counters") float FillDuration = .25f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Counters") float MoneyDuration = .35f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Counters") float GainGroupWindow = .15f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Counters", meta=(ClampMin="0")) float DamageTrailDelay = .25f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Counters", meta=(ClampMin="0.01")) float DamageTrailDuration = .5f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects", meta=(ClampMin="0.01")) float TravelDuration = .45f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects", meta=(ClampMin="0.01")) float FloatingTextDuration = .9f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects") int32 MaxParticles = 48;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects") int32 MaxFloatingTexts = 8;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects") FLinearColor Gold = FLinearColor(1,.55f,.04f,1);
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects") FLinearColor Danger = FLinearColor(1,.04f,.02f,1);
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects") FLinearColor Heal = FLinearColor(.2f,1,.3f,1);
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0",ClampMax="1")) float Volume = .5f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0",ClampMax="0.05")) float PitchVariation = .05f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0.08")) float PickupSoundInterval = .08f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TMap<FName,TObjectPtr<USoundBase>> Sounds;
};
