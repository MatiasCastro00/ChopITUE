#include "UI/Phone/ChopItPhoneIndicatorConfig.h"
#include "UI/Phone/ChopItPhoneIndicatorWidget.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UChopItPhoneIndicatorConfig::IsValidConfig() const
{
	return !Label.IsEmpty() && FMath::IsFinite(ScreenMargin) && ScreenMargin >= 0
		&& FMath::IsFinite(WorldWidth) && WorldWidth > 0
		&& FMath::IsFinite(WorldHeight) && WorldHeight >= 0
		&& FMath::IsFinite(WorldRightOffset)
		&& FMath::IsFinite(UpdatePeriod) && UpdatePeriod > 0
		&& FMath::IsFinite(EntranceDuration) && EntranceDuration > 0
		&& FMath::IsFinite(ExitDuration) && ExitDuration > 0
		&& FMath::IsFinite(TypeDuration) && TypeDuration >= 0
		&& FMath::IsFinite(BounceOvershoot) && BounceOvershoot >= 0
		&& (!WidgetClass || !WidgetClass->HasAnyClassFlags(CLASS_Abstract));
}

#if WITH_EDITOR
EDataValidationResult UChopItPhoneIndicatorConfig::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	if (IsValidConfig()) return CombineDataValidationResults(Parent, EDataValidationResult::Valid);
	Context.AddError(FText::FromString(TEXT("Phone indicator needs a label, positive distance/timing, valid scale range and concrete widget class.")));
	return EDataValidationResult::Invalid;
}
#endif
