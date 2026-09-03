#include "Camera/ChopItCameraComponent.h"
#include "Camera/ChopItCameraCue.h"
#include "Components/StaticMeshComponent.h"
#include "Core/CameraAsset.h"
#include "Core/CameraRigAsset.h"
#include "Core/CameraShakeAsset.h"
#include "Directors/SingleCameraDirector.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Misc/AutomationTest.h"
#include "StateTree.h"
#include "Engine/StaticMesh.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItCameraOrbitLimitsTest,
	"ChopIt.Camera.OrbitLimitsAndZoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItCameraOrbitLimitsTest::RunTest(const FString&)
{
	UChopItCameraComponent* Camera = NewObject<UChopItCameraComponent>();
	TestNotNull(TEXT("Camera component"), Camera);
	const float InitialPitch = Camera->GetGameplayView().Pitch;
	Camera->AddGamepadLookInput(FVector2D(0.0, 0.1), 0.1f);
	TestTrue(TEXT("Positive vertical input looks up by default"), Camera->GetGameplayView().Pitch > InitialPitch);
	Camera->AddLookInput(FVector2D(10.0, 100.0), 1.0f);
	TestEqual(TEXT("Pitch reaches the near-vertical safety pole"), Camera->GetGameplayView().Pitch, UChopItCameraComponent::MaxPitch);
	Camera->AddLookInput(FVector2D(10.0, -100.0), 1.0f);
	TestEqual(TEXT("Pitch clamps at lower limit"), Camera->GetGameplayView().Pitch, UChopItCameraComponent::MinPitch);
	TestTrue(TEXT("Yaw remains free and changes"), !FMath::IsNearlyEqual(Camera->GetGameplayView().Yaw, -45.0f));
	TestEqual(TEXT("Minimum zoom contract"), UChopItCameraComponent::MinDistance, 550.0f);
	TestEqual(TEXT("Maximum zoom contract"), UChopItCameraComponent::MaxDistance, 1400.0f);
	TestEqual(TEXT("Zoom step contract"), UChopItCameraComponent::ZoomStep, 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItCameraRequestStackTest,
	"ChopIt.Camera.RequestStackAndRestoration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItCameraRequestStackTest::RunTest(const FString&)
{
	UChopItCameraComponent* Camera = NewObject<UChopItCameraComponent>();
	UChopItCameraCue* Contextual = NewObject<UChopItCameraCue>();
	Contextual->Mode = EChopItCameraMode::Scripted;
	Contextual->Priority = 100;
	UChopItCameraCue* Cinematic = NewObject<UChopItCameraCue>();
	Cinematic->Mode = EChopItCameraMode::Cinematic;
	Cinematic->Priority = 400;

	Camera->AddLookInput(FVector2D(0.4, 0.2), 0.5f);
	const FChopItGameplayCameraView Before = Camera->GetGameplayView();
	const FChopItCameraHandle ContextualHandle = Camera->PushCue(Contextual, nullptr, nullptr);
	const FChopItCameraHandle CinematicHandle = Camera->PushCue(Cinematic, nullptr, nullptr);
	TestTrue(TEXT("Opaque handles are unique"), ContextualHandle.IsValid() && CinematicHandle.IsValid() && ContextualHandle.Id != CinematicHandle.Id);
	TestEqual(TEXT("Higher priority wins"), Camera->GetActiveMode(), EChopItCameraMode::Cinematic);
	Camera->PopCue(CinematicHandle, false);
	TestEqual(TEXT("Nested pop returns to previous cue"), Camera->GetActiveMode(), EChopItCameraMode::Scripted);
	Camera->PopCue(ContextualHandle, false);
	TestEqual(TEXT("Final pop returns to gameplay"), Camera->GetActiveMode(), EChopItCameraMode::GameplayOrbit);
	const FChopItGameplayCameraView After = Camera->GetGameplayView();
	TestTrue(TEXT("Yaw restores exactly"), Before.Yaw == After.Yaw);
	TestTrue(TEXT("Pitch restores exactly"), Before.Pitch == After.Pitch);
	TestTrue(TEXT("Distance restores exactly"), Before.Distance == After.Distance);
	Camera->PopCue(ContextualHandle, false);
	TestEqual(TEXT("Repeated pop is safe"), Camera->GetActiveMode(), EChopItCameraMode::GameplayOrbit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItCameraAssetsTest,
	"ChopIt.Camera.AssetsAndStateTreeDirector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItCameraAssetsTest::RunTest(const FString&)
{
	const UCameraAsset* CameraAsset = LoadObject<UCameraAsset>(nullptr, TEXT("/Game/ChopIt/Presentation/Camera/CA_PlayerCameras.CA_PlayerCameras"));
	const UStateTree* StateTree = LoadObject<UStateTree>(nullptr, TEXT("/Game/ChopIt/Presentation/Camera/ST_CameraDirector.ST_CameraDirector"));
	TestNotNull(TEXT("Player camera asset exists"), CameraAsset);
	TestNotNull(TEXT("Camera StateTree exists"), StateTree);
	if (!CameraAsset || !StateTree) return false;

	const USingleCameraDirector* Director = Cast<USingleCameraDirector>(CameraAsset->GetCameraDirector());
	TestNotNull(TEXT("Camera asset has a stable single-rig host director"), Director);
	if (Director)
	{
		TestNotNull(TEXT("Host director references the gameplay base rig"), Director->CameraRig.Get());
		TestEqual(TEXT("Host director uses the gameplay orbit rig"), Director->CameraRig->GetName(), FString(TEXT("CR_GameplayOrbit")));
	}

	for (const TCHAR* Path :
	{
		TEXT("/Game/ChopIt/Presentation/Camera/Rigs/CR_GameplayOrbit.CR_GameplayOrbit"),
		TEXT("/Game/ChopIt/Presentation/Camera/Rigs/CR_DialogueAnchor.CR_DialogueAnchor"),
		TEXT("/Game/ChopIt/Presentation/Camera/Rigs/CR_Cinematic.CR_Cinematic"),
		TEXT("/Game/ChopIt/Presentation/Camera/Rigs/CR_Death.CR_Death")
	})
	{
		TestNotNull(FString::Printf(TEXT("Camera rig loads: %s"), Path), LoadObject<UCameraRigAsset>(nullptr, Path));
	}
	for (const TCHAR* Path :
	{
		TEXT("/Game/ChopIt/Presentation/Camera/Shakes/CS_Normal.CS_Normal"),
		TEXT("/Game/ChopIt/Presentation/Camera/Shakes/CS_Critical.CS_Critical"),
		TEXT("/Game/ChopIt/Presentation/Camera/Shakes/CS_HeavyImpact.CS_HeavyImpact")
	})
	{
		TestNotNull(FString::Printf(TEXT("Camera shake loads: %s"), Path), LoadObject<UCameraShakeAsset>(nullptr, Path));
	}

	UMaterial* OcclusionMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Game/ChopIt/Presentation/Camera/Materials/M_CameraFoliageOcclusion.M_CameraFoliageOcclusion"));
	TestNotNull(TEXT("Generic camera occlusion material exists"), OcclusionMaterial);
	if (OcclusionMaterial)
	{
		TestEqual(TEXT("Occlusion uses stable masked rendering"), OcclusionMaterial->BlendMode, BLEND_Masked);
		const bool bUsesTemporalDither = OcclusionMaterial->GetExpressionCollection().Expressions.ContainsByPredicate(
			[](const TObjectPtr<UMaterialExpression>& Expression)
			{
				const UMaterialExpressionMaterialFunctionCall* FunctionCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expression);
				return FunctionCall && FunctionCall->MaterialFunction && FunctionCall->MaterialFunction->GetName().Contains(TEXT("DitherTemporalAA"));
			});
		TestTrue(TEXT("Occlusion mask is driven by DitherTemporalAA"), bUsesTemporalDither);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItCameraOcclusionOverlayTest,
	"ChopIt.Camera.OcclusionHidesAndRestoresOverlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItCameraOcclusionOverlayTest::RunTest(const FString&)
{
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* OriginalMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	UMaterialInterface* DitherMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ChopIt/Presentation/Camera/Materials/M_CameraFoliageOcclusion.M_CameraFoliageOcclusion"));
	UMaterialInterface* OutlineMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ChopIt/Art/Materials/PSX_Materials/MI_PSX_Outline_Jittering.MI_PSX_Outline_Jittering"));
	TestNotNull(TEXT("Test cube mesh"), CubeMesh);
	TestNotNull(TEXT("Original material"), OriginalMaterial);
	TestNotNull(TEXT("Dither material"), DitherMaterial);
	TestNotNull(TEXT("Outline material"), OutlineMaterial);
	if (!CubeMesh || !OriginalMaterial || !DitherMaterial || !OutlineMaterial) return false;

	UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>();
	MeshComponent->SetStaticMesh(CubeMesh);
	MeshComponent->SetMaterial(0, OriginalMaterial);
	MeshComponent->SetOverlayMaterial(OutlineMaterial);

	FChopItOccludedPrimitiveState State;
	State.CaptureAndApplyOcclusion(*MeshComponent, *DitherMaterial);
	TestEqual(TEXT("Dither replaces the base material"), MeshComponent->GetMaterial(0), DitherMaterial);
	TestNull(TEXT("Outline is disabled while dithering"), MeshComponent->GetOverlayMaterial());

	State.Restore();
	TestEqual(TEXT("Original base material is restored"), MeshComponent->GetMaterial(0), OriginalMaterial);
	TestEqual(TEXT("Original outline is restored"), MeshComponent->GetOverlayMaterial(), OutlineMaterial);
	return true;
}

#endif
