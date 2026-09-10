#pragma once

#include "GameFramework/Actor.h"
#include "ChopItAxeSwingTrail.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Brief, layered brush-slash cue for automatic axe attacks. */
UCLASS()
class CHOPITPRESENTATION_API AChopItAxeSwingTrail final : public AActor
{
	GENERATED_BODY()

public:
	AChopItAxeSwingTrail();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeTrail(const FVector& Forward, float Range, bool bHit, float EffectsDensity);

private:
	UMaterialInstanceDynamic* ConfigureLayer(
		UStaticMeshComponent* Layer,
		const FLinearColor& Color,
		float Intensity,
		float EdgePower,
		float Opacity,
		float MaskScale,
		float MaskOffset,
		float Breakup,
		const FLinearColor& MaskWeights);
	void UpdateTrail();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> AfterimageLayer;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MainLayer;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> InnerLayer;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNiagaraComponent> DetailParticles;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> AfterimageMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MainMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> InnerMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "ChopIt|VFX")
	TSoftObjectPtr<UStaticMesh> MainMeshAsset;

	UPROPERTY(EditDefaultsOnly, Category = "ChopIt|VFX")
	TSoftObjectPtr<UStaticMesh> InnerMeshAsset;

	UPROPERTY(EditDefaultsOnly, Category = "ChopIt|VFX")
	TSoftObjectPtr<UStaticMesh> AfterimageMeshAsset;

	UPROPERTY(EditDefaultsOnly, Category = "ChopIt|VFX")
	TSoftObjectPtr<UMaterialInterface> AdditiveMaterialAsset;

	UPROPERTY(EditDefaultsOnly, Category = "ChopIt|VFX")
	TSoftObjectPtr<UMaterialInterface> AfterimageMaterialAsset;

	UPROPERTY(EditDefaultsOnly, Category = "ChopIt|VFX")
	TSoftObjectPtr<UNiagaraSystem> DetailsSystemAsset;

	float Age = 0.0f;
	float Duration = 0.36f;
	float Density = 1.0f;
	float MainDuration = 0.25f;
	float InnerDuration = 0.29f;
	float AfterimageDuration = 0.36f;
	FVector BaseScale = FVector::OneVector;
	FRotator BaseRotation = FRotator::ZeroRotator;
	bool bWasHit = false;
};
