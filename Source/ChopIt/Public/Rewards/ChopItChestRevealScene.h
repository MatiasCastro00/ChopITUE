#pragma once
#include "GameFramework/Actor.h"
#include "ChopItChestRevealScene.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UNiagaraComponent;

/** Private, capture-only 3D stage. Updated explicitly by the HUD while gameplay is paused. */
UCLASS(Transient, NotBlueprintable)
class CHOPIT_API AChopItChestRevealScene : public AActor
{
	GENERATED_BODY()
public:
	AChopItChestRevealScene();
	void InitializeScene();
	void UpdatePresentation(float Time, float Duration, int32 Tier);
	UTextureRenderTarget2D* GetTexture() const { return Target; }
private:
	UPROPERTY() TObjectPtr<USceneComponent> Model;
	UPROPERTY() TObjectPtr<USceneComponent> Hinge;
	UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> Target;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Sparks;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Rays;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Glow;
	UPROPERTY() TObjectPtr<UMaterialInterface> LightMaterialAsset;
	UPROPERTY() TObjectPtr<UPointLightComponent> InnerLight;
	UPROPERTY() TObjectPtr<UNiagaraComponent> Finale;
	float LastTime = 0.f;
	bool bBurst = false;
};
