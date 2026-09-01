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
	/** World-space composition offset added to the resolved subject focus point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera") FVector SubjectFocusOffset = FVector::ZeroVector;
	/** Ignore auxiliary component bounds (ropes, labels, weapons) and focus from the actor origin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera") bool bUseActorLocationForSubject = false;
};
