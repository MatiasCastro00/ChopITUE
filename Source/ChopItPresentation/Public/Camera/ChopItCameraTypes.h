#pragma once

#include "CoreMinimal.h"
#include "ChopItCameraTypes.generated.h"

UENUM(BlueprintType)
enum class EChopItCameraMode : uint8
{
	GameplayOrbit,
	Scripted,
	Cinematic,
	Death
};

UENUM(BlueprintType)
enum class EChopItCameraDurationPolicy : uint8
{
	Manual,
	Timed
};

UENUM(BlueprintType, meta=(Bitflags))
enum class EChopItCameraInputLock : uint8
{
	None = 0,
	Camera = 1 << 0,
	Movement = 1 << 1,
	Actions = 1 << 2
};
ENUM_CLASS_FLAGS(EChopItCameraInputLock);

USTRUCT(BlueprintType)
struct CHOPITPRESENTATION_API FChopItCameraHandle
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	FGuid Id;

	FChopItCameraHandle() = default;
	explicit FChopItCameraHandle(const FGuid& InId) : Id(InId) {}
	bool IsValid() const { return Id.IsValid(); }
	void Invalidate() { Id.Invalidate(); }
	friend bool operator==(const FChopItCameraHandle& A, const FChopItCameraHandle& B) { return A.Id == B.Id; }
};

USTRUCT(BlueprintType)
struct CHOPITPRESENTATION_API FChopItGameplayCameraView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Camera") float Yaw = -45.0f;
	UPROPERTY(BlueprintReadOnly, Category="Camera") float Pitch = -32.0f;
	UPROPERTY(BlueprintReadOnly, Category="Camera") float Distance = 850.0f;
};
