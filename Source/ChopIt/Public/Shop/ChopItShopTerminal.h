#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/ChopItInteractable.h"
#include "ChopItShopTerminal.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UChopItWeaponDefinition;

/** Diegetic terminal which opens the player-owned shop session. */
UCLASS(Blueprintable)
class CHOPIT_API AChopItShopTerminal final : public AActor, public IChopItInteractable
{
	GENERATED_BODY()

public:
	AChopItShopTerminal();
	virtual void BeginPlay() override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool Interact_Implementation(AActor* Interactor) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Shop")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Shop")
	TObjectPtr<UStaticMeshComponent> TerminalVisual;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Shop")
	TObjectPtr<UTextRenderComponent> TerminalLabel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UChopItWeaponDefinition>> Catalog;
};
