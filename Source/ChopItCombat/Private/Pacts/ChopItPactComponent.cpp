#include "Pacts/ChopItPactComponent.h"
#include "Pacts/ChopItPactDefinition.h"
bool UChopItPactComponent::OpenOffers(const TArray<UChopItPactDefinition*>& NewOffers)
{
	if (HasActiveOffer()) return false;
	for (UChopItPactDefinition* Pact : NewOffers) if (Pact && !AcceptedIds.Contains(Pact->PactId)) ActiveOffers.Add(Pact);
	OnOffersChanged.Broadcast(); return HasActiveOffer();
}
bool UChopItPactComponent::SelectOffer(int32 Index, UChopItCombatStatsComponent* Stats)
{
	if (!ActiveOffers.IsValidIndex(Index) || !Stats) return false;
	UChopItPactDefinition* Pact=ActiveOffers[Index];
	if (!Pact || AcceptedIds.Contains(Pact->PactId)) return false;
	for(const FChopItStatModifier& Modifier:Pact->Modifiers) Stats->AddModifier(Modifier);
	AcceptedIds.Add(Pact->PactId); Curse+=FMath::Max(1,Pact->CurseIncrease); ActiveOffers.Reset(); OnOffersChanged.Broadcast(); return true;
}
