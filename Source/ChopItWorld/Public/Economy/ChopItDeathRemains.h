#pragma once

#include "GameFramework/Actor.h"
#include "ChopItDeathRemains.generated.h"

class USceneComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;

/** Persistent, physics-driven remains emitted by the machine's death sequence. */
UCLASS()
class CHOPITWORLD_API AChopItDeathRemains final : public AActor
{
	GENERATED_BODY()

public:
	AChopItDeathRemains();
	void InitializeRemains(const FVector& EjectionDirection);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BloodMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MeatMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> BloodDropletMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> MeatFragmentMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> FragmentBaseMaterial;
};
