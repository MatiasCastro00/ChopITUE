#include "Camera/ChopItCameraComponent.h"
#include "Camera/ChopItCameraCue.h"
#include "Core/CameraAsset.h"
#include "Core/CameraRigAsset.h"
#include "Core/CameraShakeAsset.h"
#include "Directors/StateTreeCameraDirector.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Misc/AutomationTest.h"
#include "StateTree.h"

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

	const UStateTreeCameraDirector* Director = Cast<UStateTreeCameraDirector>(CameraAsset->GetCameraDirector());
	TestNotNull(TEXT("Camera asset is driven by StateTree director"), Director);
	if (Director) TestTrue(TEXT("Director references ChopIt camera StateTree"), Director->StateTreeReference.GetStateTree() == StateTree);

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

#endif
