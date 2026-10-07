#include "Items/ChopItItemEffect.h"
#include "Items/ChopItItemComponent.h"
#include "Items/ChopItItemDataAsset.h"

UWorld* UChopItItemEffect::GetWorld() const { return Inventory ? Inventory->GetWorld() : nullptr; }
float UChopItItemEffect::GetStackedValue() const
{
	return UChopItItemComponent::CalculateStackedValue(BaseValue, Stacks, Definition ? Definition->StackDecay : 0.5f);
}
