#include "Phone/ChopItPhoneVariantPool.h"
#include "Phone/ChopItPhoneVariantData.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UChopItPhoneVariantPool::IsValidConfig() const
{
	if (Variants.IsEmpty()) return false;
	TSet<FName> Ids;
	float Total = 0.0f;
	for (const UChopItPhoneVariantData* Variant : Variants)
	{
		if (!Variant || !Variant->IsValidConfig() || Ids.Contains(Variant->VariantId)) return false;
		Ids.Add(Variant->VariantId);
		Total += Variant->Weight;
	}
	return FMath::IsFinite(Total) && Total > 0.0f;
}

float UChopItPhoneVariantPool::GetTotalWeight() const
{
	float Total = 0.0f;
	for (const UChopItPhoneVariantData* Variant : Variants)
		if (Variant && FMath::IsFinite(Variant->Weight) && Variant->Weight > 0.0f) Total += Variant->Weight;
	return Total;
}

const UChopItPhoneVariantData* UChopItPhoneVariantPool::SelectByRoll(float Roll) const
{
	if (!IsValidConfig() || !FMath::IsFinite(Roll)) return nullptr;
	float Remaining = FMath::Clamp(Roll, 0.0f, GetTotalWeight());
	const UChopItPhoneVariantData* LastPositive = nullptr;
	for (const UChopItPhoneVariantData* Variant : Variants)
	{
		if (Variant->Weight <= 0.0f) continue;
		LastPositive = Variant;
		if (Remaining < Variant->Weight) return Variant;
		Remaining -= Variant->Weight;
	}
	return LastPositive;
}

const UChopItPhoneVariantData* UChopItPhoneVariantPool::Select(FRandomStream& Random) const
{
	return SelectByRoll(Random.FRandRange(0.0f, GetTotalWeight()));
}

#if WITH_EDITOR
EDataValidationResult UChopItPhoneVariantPool::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	if (IsValidConfig()) return CombineDataValidationResults(Parent, EDataValidationResult::Valid);
	Context.AddError(FText::FromString(TEXT("Phone variant pool needs unique valid variants and a positive total weight.")));
	return EDataValidationResult::Invalid;
}
#endif
