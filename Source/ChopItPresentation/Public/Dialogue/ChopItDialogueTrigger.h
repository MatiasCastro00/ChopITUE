#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/ChopItInteractable.h"
#include "Dialogue/ChopItDialogueTypes.h"
#include "ChopItDialogueTrigger.generated.h"

class UChopItDialogueSequence;
class UStaticMeshComponent;

/** Reusable world interaction that starts a data-authored dialogue. */
UCLASS()
class CHOPITPRESENTATION_API AChopItDialogueTrigger final : public AActor, public IChopItInteractable
{
	GENERATED_BODY()
public:
	AChopItDialogueTrigger();
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool Interact_Implementation(AActor* Interactor) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") TObjectPtr<UChopItDialogueSequence> Sequence;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") EChopItDialogueStartPolicy StartPolicy = EChopItDialogueStartPolicy::Queue;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FName TriggerBinding = TEXT("Speaker");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") FName InteractorBinding = TEXT("Player");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue") TObjectPtr<AActor> CameraAnchor;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
};

