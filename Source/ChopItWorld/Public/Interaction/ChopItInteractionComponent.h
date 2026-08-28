#pragma once

#include "Components/ActorComponent.h"
#include "ChopItInteractionComponent.generated.h"

/** Input-facing interaction seam; concrete interactables arrive with the hub phase. */
UCLASS(ClassGroup = (ChopIt), BlueprintType, meta = (BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItInteractionComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UChopItInteractionComponent();

	UFUNCTION(BlueprintCallable, Category = "ChopIt|Interaction")
	bool TryInteract();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Interaction", meta = (ClampMin = "0.0"))
	float InteractionRange = 300.0f;
};
