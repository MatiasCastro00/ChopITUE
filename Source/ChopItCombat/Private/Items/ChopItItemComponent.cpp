#include "Items/ChopItItemComponent.h"
#include "Items/ChopItItemDataAsset.h"
#include "Items/ChopItItemEffect.h"
#include "Items/ChopItItemEventSubsystem.h"
#include "Engine/World.h"

UChopItItemComponent::UChopItItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItItemComponent::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetSubsystem<UChopItItemEventSubsystem>()->OnEvent.AddUObject(this, &ThisClass::ReceiveEvent);
	TGuardValue<bool> Guard(bMutating, true);
	TArray<FName> Ids;
	OwnedItems.GetKeys(Ids);
	for (FName Id : Ids) ActivateEffects(Id);
}

void UChopItItemComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	bEndingPlay = true;
	if (auto* Events = GetWorld()->GetSubsystem<UChopItItemEventSubsystem>()) Events->OnEvent.RemoveAll(this);
	ClearItems();
	Super::EndPlay(Reason);
}

float UChopItItemComponent::CalculateStackedValue(float BaseValue, int32 StackCount, float StackDecay)
{
	if (StackCount <= 0 || !FMath::IsFinite(BaseValue) || !FMath::IsFinite(StackDecay)) return 0.f;
	const double Decay = FMath::Clamp(static_cast<double>(StackDecay), 0.0, 1.0);
	return Decay == 1.0 ? BaseValue * StackCount : BaseValue * (1.0 - FMath::Pow(Decay, StackCount)) / (1.0 - Decay);
}

void UChopItItemComponent::ActivateEffects(FName Id)
{
	FChopItOwnedItem& Entry = OwnedItems.FindChecked(Id);
	if (!Entry.Effects.IsEmpty()) return;
	for (UChopItItemEffect* Template : Entry.Item->Effects)
	{
		if (!Template || Template->GetClass()->HasAnyClassFlags(CLASS_Abstract)) continue;
		auto* Effect = DuplicateObject<UChopItItemEffect>(Template, this);
		Effect->Inventory = this;
		Effect->Definition = Entry.Item;
		Effect->Stacks = Entry.Stacks;
		Entry.Effects.Add(Effect);
		Effect->OnActivated();
	}
}

bool UChopItItemComponent::AddItem(UChopItItemDataAsset* Item, int32 Count)
{
	if (bMutating || bClearing || bEndingPlay || !IsValid(Item) || Item->ItemId.IsNone() || Count <= 0) return false;
	FChopItOwnedItem* Existing = OwnedItems.Find(Item->ItemId);
	if (Existing && (Existing->Item != Item || Existing->Stacks > MAX_int32 - Count)) return false;
	int32 NewCount;
	{
		TGuardValue<bool> Guard(bMutating, true);
		FChopItOwnedItem& Entry = OwnedItems.FindOrAdd(Item->ItemId);
		Entry.Item = Item;
		Entry.Stacks += Count;
		NewCount = Entry.Stacks;
		if (HasBegunPlay())
		{
			if (!Existing)
			{
				ActivateEffects(Item->ItemId);
			}
			else
			{
				for (UChopItItemEffect* Effect : Entry.Effects)
				{
					Effect->Stacks = NewCount;
					Effect->OnStacksChanged();
				}
			}
		}
	}
	OnInventoryChanged.Broadcast(Item, NewCount);
	return true;
}

bool UChopItItemComponent::RemoveItem(FName ItemId, int32 Count)
{
	if (bMutating || Count <= 0) return false;
	FChopItOwnedItem* Entry = OwnedItems.Find(ItemId);
	if (!Entry) return false;
	UChopItItemDataAsset* Item = Entry->Item;
	const int32 Remaining = FMath::Max(0, Entry->Stacks - Count);
	{
		TGuardValue<bool> Guard(bMutating, true);
		Entry->Stacks = Remaining;
		for (UChopItItemEffect* Effect : Entry->Effects)
		{
			Effect->Stacks = Remaining;
			if (Remaining) Effect->OnStacksChanged(); else Effect->OnDeactivated();
		}
		if (!Remaining) OwnedItems.Remove(ItemId);
	}
	OnInventoryChanged.Broadcast(Item, Remaining);
	return true;
}

void UChopItItemComponent::ClearItems()
{
	if (bMutating || bClearing) return;
	TGuardValue<bool> Guard(bClearing, true);
	TArray<FName> Ids;
	OwnedItems.GetKeys(Ids);
	for (FName Id : Ids) RemoveItem(Id, MAX_int32);
}
int32 UChopItItemComponent::GetStackCount(FName Id) const
{
	const auto* Entry = OwnedItems.Find(Id);
	return Entry ? Entry->Stacks : 0;
}

TArray<FChopItOwnedItem> UChopItItemComponent::GetItems() const
{
	TArray<FChopItOwnedItem> Result;
	OwnedItems.GenerateValueArray(Result);
	return Result;
}
FGameplayTagContainer UChopItItemComponent::GetOwnedTags() const
{
	FGameplayTagContainer Result;
	for (const auto& Pair : OwnedItems) Result.AppendTags(Pair.Value.Item->Tags);
	return Result;
}
void UChopItItemComponent::ReceiveEvent(const FChopItItemEventContext& Context)
{
	if (bEndingPlay || (Context.Recipient && Context.Recipient != GetOwner())) return;
	// Snapshot permits listeners to remove items while dispatching; deactivated effects are skipped.
	const auto Snapshot = GetItems();
	for (const auto& Entry : Snapshot)
	{
		for (UChopItItemEffect* Effect : Entry.Effects)
		{
			if (Effect->Stacks > 0 && Effect->Events.Contains(Context.Event)) Effect->HandleEvent(Context);
		}
	}
	OnItemEvent.Broadcast(Context);
}
