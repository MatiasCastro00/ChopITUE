#include "Progression/ChopItUpgradeOfferComponent.h"

#include "Combat/ChopItCombatStatsComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Progression/ChopItExperienceComponent.h"
#include "Progression/ChopItUpgradeDefinition.h"

UChopItUpgradeOfferComponent::UChopItUpgradeOfferComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItUpgradeOfferComponent::BeginPlay()
{
	Super::BeginPlay();
	LoadDefaultCatalog();
	if (UChopItExperienceComponent* Experience = ResolveExperience())
	{
		Experience->OnLevelUpQueued.AddUniqueDynamic(this, &UChopItUpgradeOfferComponent::HandleLevelUpQueued);
	}
}

void UChopItUpgradeOfferComponent::LoadDefaultCatalog()
{
	if (!Catalog.IsEmpty())
	{
		return;
	}
	static const TCHAR* Names[] =
	{
		TEXT("Filo"), TEXT("Ritmo"), TEXT("Alcance"), TEXT("Critico"),
		TEXT("Botas"), TEXT("Furia"), TEXT("Precision"), TEXT("Gigante")
	};
	for (const TCHAR* Name : Names)
	{
		const FString Path = FString::Printf(
			TEXT("/Game/ChopIt/Progression/Upgrades/DA_Upgrade_%s.DA_Upgrade_%s"), Name, Name);
		if (UChopItUpgradeDefinition* Upgrade = LoadObject<UChopItUpgradeDefinition>(nullptr, *Path))
		{
			Catalog.Add(Upgrade);
		}
	}
}

void UChopItUpgradeOfferComponent::HandleLevelUpQueued(const int32 NewLevel, const int32)
{
	if (!HasActiveOffer())
	{
		BuildOffers(NewLevel, OfferSeed + OfferSequence++ * 7919);
	}
}

void UChopItUpgradeOfferComponent::BuildOffers(const int32 Level, const int32 Seed)
{
	ActiveOffers.Reset();
	TArray<UChopItUpgradeDefinition*> Candidates;
	for (UChopItUpgradeDefinition* Upgrade : Catalog)
	{
		if (IsValid(Upgrade) && !Upgrade->UpgradeId.IsNone()
			&& GetStackCount(Upgrade->UpgradeId) < FMath::Max(1, Upgrade->MaxStacks))
		{
			Candidates.Add(Upgrade);
		}
	}

	FRandomStream Random(Seed ^ (Level * 104729));
	while (!Candidates.IsEmpty() && ActiveOffers.Num() < OfferCount)
	{
		float TotalWeight = 0.0f;
		for (const UChopItUpgradeDefinition* Candidate : Candidates)
		{
			TotalWeight += FMath::Max(0.001f, Candidate->OfferWeight);
		}
		float Roll = Random.FRandRange(0.0f, TotalWeight);
		int32 PickedIndex = Candidates.Num() - 1;
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			Roll -= FMath::Max(0.001f, Candidates[Index]->OfferWeight);
			if (Roll <= 0.0f)
			{
				PickedIndex = Index;
				break;
			}
		}
		ActiveOffers.Add(Candidates[PickedIndex]);
		Candidates.RemoveAtSwap(PickedIndex);
	}

	bSelectionLocked = false;
	OnOffersChanged.Broadcast();
	SetPaused(!ActiveOffers.IsEmpty());
}

bool UChopItUpgradeOfferComponent::SelectOffer(const int32 OfferIndex)
{
	if (bSelectionLocked || !ActiveOffers.IsValidIndex(OfferIndex))
	{
		return false;
	}
	bSelectionLocked = true;
	UChopItExperienceComponent* Experience = ResolveExperience();
	UChopItCombatStatsComponent* Stats = ResolveStats();
	if (!Experience || !Stats || !ApplyUpgrade(ActiveOffers[OfferIndex], Stats))
	{
		bSelectionLocked = false;
		return false;
	}

	Experience->ConsumePendingLevelUp();
	ActiveOffers.Reset();
	OnOffersChanged.Broadcast();
	if (Experience->GetPendingLevelUps() > 0)
	{
		BuildOffers(Experience->GetLevel(), OfferSeed + OfferSequence++ * 7919);
	}
	else
	{
		SetPaused(false);
	}
	return true;
}

bool UChopItUpgradeOfferComponent::ApplyUpgrade(
	UChopItUpgradeDefinition* Upgrade,
	UChopItCombatStatsComponent* Stats)
{
	if (!IsValid(Upgrade) || !IsValid(Stats) || Upgrade->UpgradeId.IsNone()
		|| GetStackCount(Upgrade->UpgradeId) >= FMath::Max(1, Upgrade->MaxStacks))
	{
		return false;
	}
	TArray<FGuid>& Handles = AppliedHandles.FindOrAdd(Upgrade->UpgradeId);
	for (const FChopItStatModifier& Modifier : Upgrade->Modifiers)
	{
		Handles.Add(Stats->AddModifier(Modifier));
	}
	StackCounts.FindOrAdd(Upgrade->UpgradeId)++;
	return true;
}

bool UChopItUpgradeOfferComponent::RemoveLastStack(
	const FName UpgradeId,
	UChopItCombatStatsComponent* Stats)
{
	int32* Count = StackCounts.Find(UpgradeId);
	TArray<FGuid>* Handles = AppliedHandles.Find(UpgradeId);
	if (!Count || *Count <= 0 || !Handles || !IsValid(Stats))
	{
		return false;
	}
	const UChopItUpgradeDefinition* Definition = nullptr;
	for (const UChopItUpgradeDefinition* Item : Catalog)
	{
		if (IsValid(Item) && Item->UpgradeId == UpgradeId)
		{
			Definition = Item;
			break;
		}
	}
	const int32 HandlesPerStack = Definition ? Definition->Modifiers.Num() : 0;
	if (HandlesPerStack <= 0 || Handles->Num() < HandlesPerStack)
	{
		return false;
	}
	for (int32 Index = 0; Index < HandlesPerStack; ++Index)
	{
		Stats->RemoveModifier(Handles->Pop());
	}
	--(*Count);
	return true;
}

int32 UChopItUpgradeOfferComponent::GetStackCount(const FName UpgradeId) const
{
	return StackCounts.FindRef(UpgradeId);
}

void UChopItUpgradeOfferComponent::SetPaused(const bool bPaused) const
{
	const APlayerState* State = Cast<APlayerState>(GetOwner());
	if (APlayerController* Controller = State ? Cast<APlayerController>(State->GetOwner()) : nullptr)
	{
		Controller->SetPause(bPaused);
	}
}

UChopItCombatStatsComponent* UChopItUpgradeOfferComponent::ResolveStats() const
{
	const APlayerState* State = Cast<APlayerState>(GetOwner());
	const APawn* Pawn = State ? State->GetPawn() : nullptr;
	return Pawn ? Pawn->FindComponentByClass<UChopItCombatStatsComponent>() : nullptr;
}

UChopItExperienceComponent* UChopItUpgradeOfferComponent::ResolveExperience() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UChopItExperienceComponent>() : nullptr;
}

#if WITH_DEV_AUTOMATION_TESTS
void UChopItUpgradeOfferComponent::SetCatalogForAutomation(const TArray<UChopItUpgradeDefinition*>& Definitions)
{
	Catalog.Reset();
	for (UChopItUpgradeDefinition* Definition : Definitions)
	{
		Catalog.Add(Definition);
	}
}

void UChopItUpgradeOfferComponent::GenerateOffersForAutomation(const int32 Level, const int32 Seed)
{
	BuildOffers(Level, Seed);
}
#endif
