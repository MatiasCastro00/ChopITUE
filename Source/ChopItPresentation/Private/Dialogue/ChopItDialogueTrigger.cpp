#include "Dialogue/ChopItDialogueTrigger.h"

#include "Components/StaticMeshComponent.h"
#include "Dialogue/ChopItDialogueAssets.h"
#include "Dialogue/ChopItDialogueSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AChopItDialogueTrigger::AChopItDialogueTrigger()
{
	PrimaryActorTick.bCanEverTick = false;
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	SetRootComponent(Visual);
	Visual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Visual->SetCollisionObjectType(ECC_WorldDynamic);
	Visual->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.25f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Mesh.Succeeded()) Visual->SetStaticMesh(Mesh.Object);
}

bool AChopItDialogueTrigger::CanInteract_Implementation(AActor* Interactor) const
{
	const APawn* Pawn = Cast<APawn>(Interactor);
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const UChopItDialogueSubsystem* Dialogue = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UChopItDialogueSubsystem>() : nullptr;
	return IsValid(Sequence) && Dialogue && !Dialogue->IsDialogueActive();
}

bool AChopItDialogueTrigger::Interact_Implementation(AActor* Interactor)
{
	APawn* Pawn = Cast<APawn>(Interactor);
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	UChopItDialogueSubsystem* Dialogue = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UChopItDialogueSubsystem>() : nullptr;
	if (!Dialogue || !Sequence) return false;
	FChopItDialogueContext Context;
	Context.Bindings.Add({TriggerBinding, this});
	Context.Bindings.Add({InteractorBinding, Interactor});
	Context.Bindings.Add({TEXT("CameraAnchor"), CameraAnchor});
	return Dialogue->StartDialogue(Sequence, Context, StartPolicy).IsValid();
}

