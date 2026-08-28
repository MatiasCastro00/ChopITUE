#pragma once

#include "UObject/Interface.h"
#include "ChopItInteractable.generated.h"

UINTERFACE(BlueprintType)
class CHOPITCORE_API UChopItInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Explicit interaction contract. Implementations validate commands against domain authority. */
class CHOPITCORE_API IChopItInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ChopIt|Interaction")
	bool CanInteract(AActor* Interactor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ChopIt|Interaction")
	bool Interact(AActor* Interactor);
};
