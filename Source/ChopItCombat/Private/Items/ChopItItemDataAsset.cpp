#include "Items/ChopItItemDataAsset.h"
#include "Items/ChopItItemEffect.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"

EDataValidationResult UChopItItemDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);
	bool bValid = true;
	auto Error = [&](const TCHAR* Message) { Context.AddError(FText::FromString(Message)); bValid = false; };
	if (ItemId.IsNone()) Error(TEXT("ItemId must be stable, unique and non-empty."));
	if (DisplayName.IsEmpty()) Error(TEXT("DisplayName must not be empty."));
	if (!FMath::IsFinite(StackDecay) || StackDecay < 0.f || StackDecay > 1.f) Error(TEXT("StackDecay must be in [0,1]."));
	if (!FMath::IsFinite(SpawnWeight) || SpawnWeight < 0.f) Error(TEXT("SpawnWeight must be finite and non-negative."));
	if (RequiredItemIds.Contains(ItemId) || RequiredItemIds.Contains(NAME_None)) Error(TEXT("Prerequisites cannot include self or an empty ID."));
	for (const UChopItItemEffect* Effect : Effects)
	{
		if (!Effect || Effect->GetClass()->HasAnyClassFlags(CLASS_Abstract)) Error(TEXT("Effects must be non-null concrete templates."));
		else if (!FMath::IsFinite(Effect->BaseValue)) Error(TEXT("Effect BaseValue must be finite."));
	}
	return bValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
