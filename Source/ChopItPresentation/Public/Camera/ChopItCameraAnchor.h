#pragma once

#include "GameFramework/Actor.h"
#include "ChopItCameraAnchor.generated.h"

class USceneComponent;

UCLASS(BlueprintType)
class CHOPITPRESENTATION_API AChopItCameraAnchor : public AActor
{
	GENERATED_BODY()
public:
	AChopItCameraAnchor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera") TObjectPtr<USceneComponent> CameraTransform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera") TObjectPtr<AActor> DefaultSubject;
};
