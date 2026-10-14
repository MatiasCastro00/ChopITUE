#include "UI/Phone/ChopItPhonePromptConfig.h"
#include "UI/Phone/ChopItPhonePromptWidget.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UChopItPhonePromptConfig::IsValidConfig() const
{
	return WidgetClass && !FallbackKeyText.IsEmpty()
		&& FMath::IsFinite(HeightAbovePhone) && FMath::IsFinite(DrawSize.X) && DrawSize.X > 0
		&& FMath::IsFinite(DrawSize.Y) && DrawSize.Y > 0
		&& FMath::IsFinite(DetectionPadding) && DetectionPadding >= 0
		&& FMath::IsFinite(CheckPeriod) && CheckPeriod > 0
		&& (!WidgetClass || !WidgetClass->HasAnyClassFlags(CLASS_Abstract));
}

#if WITH_EDITOR
EDataValidationResult UChopItPhonePromptConfig::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	if (IsValidConfig()) return CombineDataValidationResults(Parent, EDataValidationResult::Valid);
	Context.AddError(FText::FromString(TEXT("Phone prompt needs text, positive draw size/check interval, valid padding and a concrete widget class.")));
	return EDataValidationResult::Invalid;
}
#endif
