#include "Camera/ChopItCameraAnchor.h"
#include "Components/SceneComponent.h"

AChopItCameraAnchor::AChopItCameraAnchor()
{
	PrimaryActorTick.bCanEverTick = false;
	CameraTransform = CreateDefaultSubobject<USceneComponent>(TEXT("CameraTransform"));
	SetRootComponent(CameraTransform);
}
