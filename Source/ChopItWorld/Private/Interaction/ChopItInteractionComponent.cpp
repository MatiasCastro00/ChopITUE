#include "Interaction/ChopItInteractionComponent.h"

#include "ChopItLogChannels.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interaction/ChopItInteractable.h"
#include "ChopItCollision.h"

namespace
{
bool HasBlueprintEventOverride(const AActor* Actor, const FName FunctionName)
{
	const UFunction* Function = Actor ? Actor->FindFunction(FunctionName) : nullptr;
	return Function && Function->GetOuter() != UChopItInteractable::StaticClass();
}

bool CanInteract(AActor* Candidate, AActor* Interactor)
{
	// UE 5.8 may resolve the inherited interface UFunction without dispatching its
	// native _Implementation. Preserve ProcessEvent for actual Blueprint overrides.
	if (!HasBlueprintEventOverride(Candidate, TEXT("CanInteract")))
	{
		if (const IChopItInteractable* Native = Cast<IChopItInteractable>(Candidate))
			return Native->CanInteract_Implementation(Interactor);
	}
	return IChopItInteractable::Execute_CanInteract(Candidate, Interactor);
}

bool Interact(AActor* Candidate, AActor* Interactor)
{
	if (!HasBlueprintEventOverride(Candidate, TEXT("Interact")))
	{
		if (IChopItInteractable* Native = Cast<IChopItInteractable>(Candidate))
			return Native->Interact_Implementation(Interactor);
	}
	return IChopItInteractable::Execute_Interact(Candidate, Interactor);
}
}

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
	ObjectQuery.AddObjectTypesToQuery(ChopItCollisionChannels::DeliveryZone);
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
		if (DistanceSquared <= BestDistanceSquared && CanInteract(Candidate, Owner))
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
	return Interact(BestActor, Owner);
}
