#include "Phone/ChopItPhoneVariantData.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UChopItPhoneVariantData::IsValidConfig() const
{
	return !VariantId.IsNone() && !DisplayName.IsEmpty()
		&& FMath::IsFinite(Weight) && Weight >= 0.0f
		&& FMath::IsFinite(RevealDuration) && RevealDuration > 0.0f
		&& FMath::IsFinite(PeakLightIntensity) && PeakLightIntensity >= 0.0f
		&& FMath::IsFinite(IdleLightIntensity) && IdleLightIntensity >= 0.0f
		&& FMath::IsFinite(LightRadius) && LightRadius > 0.0f
		&& FMath::IsFinite(AccentColor.R) && FMath::IsFinite(AccentColor.G)
		&& FMath::IsFinite(AccentColor.B) && FMath::IsFinite(AccentColor.A);
}

#if WITH_EDITOR
EDataValidationResult UChopItPhoneVariantData::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	if (IsValidConfig()) return CombineDataValidationResults(Parent, EDataValidationResult::Valid);
	Context.AddError(FText::FromString(TEXT("Phone variant needs a unique ID, display name, nonnegative weight and valid reveal settings.")));
	return EDataValidationResult::Invalid;
}
#endif
