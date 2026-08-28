#include "Interaction/ChopItInteractionComponent.h"

#include "ChopItLogChannels.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interaction/ChopItInteractable.h"

UChopItInteractionComponent::UChopItInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UChopItInteractionComponent::TryInteract()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return false;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ChopItInteraction), false, Owner);
	World->OverlapMultiByObjectType(
		Overlaps,
		Owner->GetActorLocation(),
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(InteractionRange),
		QueryParams);

	AActor* BestActor = nullptr;
	double BestDistanceSquared = FMath::Square(static_cast<double>(InteractionRange));
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!IsValid(Candidate) || Candidate == Owner || !Candidate->Implements<UChopItInteractable>())
		{
			continue;
		}
		const double DistanceSquared = FVector::DistSquared(Owner->GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSquared <= BestDistanceSquared && IChopItInteractable::Execute_CanInteract(Candidate, Owner))
		{
			BestActor = Candidate;
			BestDistanceSquared = DistanceSquared;
		}
	}

	if (!BestActor)
	{
		UE_LOG(LogChopIt, Display, TEXT("No valid interactable within %.0f cm."), InteractionRange);
		return false;
	}
	return IChopItInteractable::Execute_Interact(BestActor, Owner);
}
