#pragma once

#include "GameFramework/GameplayCamerasPlayerCameraManager.h"
#include "ChopItPlayerCameraManager.generated.h"

/**
 * Bridges ChopIt's anchor-driven dialogue pose into the final Gameplay Cameras
 * cache consumed by the viewport.
 */
UCLASS(NotPlaceable)
class CHOPIT_API AChopItPlayerCameraManager final : public AGameplayCamerasPlayerCameraManager
{
	GENERATED_BODY()

protected:
	virtual void DoUpdateCamera(float DeltaTime) override;
};
