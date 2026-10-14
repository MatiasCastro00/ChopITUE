#pragma once

#include "GameFramework/Actor.h"
#include "ChopItPhoneDialogueScene.generated.h"

class AChopItMysteryPhone;
class UChopItPhoneVariantData;
class USceneCaptureComponent2D;
class UStaticMeshComponent;
class UPointLightComponent;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;

/** Capture-only phone stage with its own UMG composite material. */
UCLASS(Transient, NotBlueprintable)
class CHOPIT_API AChopItPhoneDialogueScene final : public AActor
{
	GENERATED_BODY()
public:
	AChopItPhoneDialogueScene();
	bool InitializeScene(const AChopItMysteryPhone* Phone, const UChopItPhoneVariantData* Variant);
	void UpdatePresentation(float Elapsed);
	UMaterialInstanceDynamic* GetCompositeMaterial() const { return CompositeMaterial; }
	UTextureRenderTarget2D* GetTexture() const { return Target; }
private:
	UPROPERTY() TObjectPtr<USceneComponent> Model;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Base;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Receiver;
	UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY() TObjectPtr<UPointLightComponent> KeyLight;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> Target;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CompositeMaterial;
};
