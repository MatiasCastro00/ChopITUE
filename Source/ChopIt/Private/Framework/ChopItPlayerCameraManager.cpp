#include "Framework/ChopItPlayerCameraManager.h"

#include "Camera/ChopItCameraComponent.h"
#include "CineCameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void AChopItPlayerCameraManager::DoUpdateCamera(const float DeltaTime)
{
	Super::DoUpdateCamera(DeltaTime);

	APawn* Pawn = PCOwner ? PCOwner->GetPawn() : nullptr;
	UChopItCameraComponent* ChopItCamera = Pawn ? Pawn->FindComponentByClass<UChopItCameraComponent>() : nullptr;
	if (!ChopItCamera || !ChopItCamera->ShouldOverridePlayerCameraManager()) return;

	const UCineCameraComponent* Output = ChopItCamera->GetOutputCameraComponent();
	if (!Output) return;

	// Preserve post process, projection and Gameplay Cameras modifier data from the
	// evaluated view. Only the authored dialogue pose and FOV replace the base rig.
	FMinimalViewInfo DialogueView = GetCameraCacheView();
	DialogueView.Location = Output->GetComponentLocation();
	DialogueView.Rotation = Output->GetComponentRotation();
	DialogueView.FOV = Output->FieldOfView;
	FillCameraCache(DialogueView);
	SetActorLocationAndRotation(DialogueView.Location, DialogueView.Rotation, false);
}
