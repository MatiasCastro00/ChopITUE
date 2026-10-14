#include "Phone/ChopItPhoneCallConfig.h"
#include "Sound/SoundBase.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UChopItPhoneCallConfig::IsValidConfig() const
{
	return IsValid(RingSound) && FMath::IsFinite(FirstCallDelaySeconds) && FirstCallDelaySeconds > 0.0f
		&& FMath::IsFinite(MinIntervalSeconds) && MinIntervalSeconds > 0.0f
		&& MinIntervalSeconds >= RingDurationSeconds
		&& FMath::IsFinite(MaxIntervalSeconds) && MaxIntervalSeconds >= MinIntervalSeconds
		&& FMath::IsFinite(RingDurationSeconds) && RingDurationSeconds > 0.0f
		&& FMath::IsFinite(RingPairSpacingSeconds) && RingPairSpacingSeconds > 0.0f
		&& FMath::IsFinite(RingStrikeSpacingSeconds) && RingStrikeSpacingSeconds > 0.0f
		&& RingStrikeSpacingSeconds < RingPairSpacingSeconds
		&& RingPairsPerCall >= 1
		&& (RingPairsPerCall - 1) * RingPairSpacingSeconds + RingStrikeSpacingSeconds < RingDurationSeconds
		&& FMath::IsFinite(FullVolumeRadius) && FullVolumeRadius >= 0.0f
		&& FMath::IsFinite(AudibleDistance) && AudibleDistance > FullVolumeRadius
		&& FMath::IsFinite(Volume) && Volume >= 0.0f && Volume <= 4.0f;
}

#if WITH_EDITOR
EDataValidationResult UChopItPhoneCallConfig::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	if (IsValidConfig()) return CombineDataValidationResults(Parent, EDataValidationResult::Valid);
	Context.AddError(FText::FromString(TEXT("Phone call needs a sound, positive timing, audible distance beyond the full-volume radius, and a valid volume.")));
	return EDataValidationResult::Invalid;
}
#endif
