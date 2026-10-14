#include "UI/Phone/ChopItPhoneDialogueScene.h"

#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneVariantData.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"

AChopItPhoneDialogueScene::AChopItPhoneDialogueScene()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Stage")));
	Model = CreateDefaultSubobject<USceneComponent>(TEXT("PhoneModel"));
	Model->SetupAttachment(RootComponent);
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	Base->SetupAttachment(Model);
	Receiver = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Receiver"));
	Receiver->SetupAttachment(Model);
	for (UStaticMeshComponent* Part : {Base.Get(), Receiver.Get()})
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		Part->SetVisibleInSceneCaptureOnly(true);
	}
	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortraitCamera"));
	Capture->SetupAttachment(RootComponent);
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	Capture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	Capture->FOVAngle = 42.0f;
	Capture->ShowFlags.SetPostProcessing(false);
	Capture->ShowFlags.SetAtmosphere(false);
	Capture->ShowFlags.SetFog(false);
	Capture->ShowFlags.SetMotionBlur(false);
	KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PhoneKeyLight"));
	KeyLight->SetupAttachment(Model);
	KeyLight->SetRelativeLocation(FVector(-90, -110, 130));
	KeyLight->SetAttenuationRadius(500.0f);
	KeyLight->SetCastShadows(false);
}

bool AChopItPhoneDialogueScene::InitializeScene(const AChopItMysteryPhone* Phone,
	const UChopItPhoneVariantData* Variant)
{
	if (!Phone || !Variant || !Phone->GetBaseVisual() || !Phone->GetReceiverVisual()) return false;
	const auto CopyPart = [](UStaticMeshComponent* Source, UStaticMeshComponent* Destination)
	{
		Destination->SetStaticMesh(Source->GetStaticMesh());
		Destination->SetRelativeTransform(Source->GetRelativeTransform());
		for (int32 Index = 0; Index < Source->GetNumMaterials(); ++Index)
			Destination->SetMaterial(Index, Source->GetMaterial(Index));
	};
	CopyPart(Phone->GetBaseVisual(), Base);
	CopyPart(Phone->GetReceiverVisual(), Receiver);
	if (!Base->GetStaticMesh() || !Receiver->GetStaticMesh()) return false;
	KeyLight->SetLightColor(Variant->AccentColor);
	KeyLight->SetIntensity(FMath::Max(Variant->IdleLightIntensity, 2000.0f));
	Target = NewObject<UTextureRenderTarget2D>(this);
	Target->ClearColor = FLinearColor(0, 0, 0, 0);
	Target->RenderTargetFormat = RTF_RGBA16f;
	Target->InitAutoFormat(768, 768);
	Target->UpdateResourceImmediate(true);
	UMaterialInterface* Composite = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/ChopIt/Phone/M_PhonePortrait_UI.M_PhonePortrait_UI"));
	if (!Composite) return false;
	CompositeMaterial = UMaterialInstanceDynamic::Create(Composite, this);
	CompositeMaterial->SetTextureParameterValue(TEXT("CaptureTexture"), Target);
	Capture->TextureTarget = Target;
	Capture->ShowOnlyActorComponents(this);
	UpdatePresentation(0.0f);
	return true;
}

void AChopItPhoneDialogueScene::UpdatePresentation(const float Elapsed)
{
	if (!Target || !Capture) return;
	Model->SetRelativeRotation(FRotator(0, Elapsed * 14.0f, 0));
	Model->SetRelativeLocation(FVector(0, 0, FMath::Sin(Elapsed * 1.8f) * 4.0f));
	const FVector CameraPosition(105, -115, 100);
	Capture->SetRelativeLocation(CameraPosition);
	Capture->SetRelativeRotation((FVector(0, 0, 43) - CameraPosition).Rotation());
	Capture->CaptureScene();
}
