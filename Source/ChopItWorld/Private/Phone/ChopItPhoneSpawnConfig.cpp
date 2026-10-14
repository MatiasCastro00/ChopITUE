#include "Phone/ChopItPhoneSpawnConfig.h"
#include "Phone/ChopItMysteryPhone.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UChopItPhoneSpawnConfig::IsValidConfig() const
{
	return FMath::IsFinite(SpawnProbability) && SpawnProbability >= 0.0f && SpawnProbability <= 1.0f
		&& MaxPhones >= 0 && MaxPhones <= 16 && FMath::IsFinite(GroundOffset)
		&& Placement.IsValid() && (!PhoneClass || !PhoneClass->HasAnyClassFlags(CLASS_Abstract));
}

#if WITH_EDITOR
EDataValidationResult UChopItPhoneSpawnConfig::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	if (IsValidConfig()) return CombineDataValidationResults(ParentResult, EDataValidationResult::Valid);
	Context.AddError(FText::FromString(TEXT("Phone spawn settings must have valid probability, count, distances, terrain limits, clearance and a concrete actor class.")));
	return EDataValidationResult::Invalid;
}
#endif
