#include "Components/ChopItCameraFacingTextComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UChopItCameraFacingTextComponent::UChopItCameraFacingTextComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	bTickInEditor = false;
	bAlwaysRenderAsText = true;
	ConfigurePsxBypass(*this);
}

void UChopItCameraFacingTextComponent::ConfigurePsxBypass(UTextRenderComponent& TextComponent)
{
	TextComponent.SetRenderCustomDepth(true);
	TextComponent.SetCustomDepthStencilValue(PsxBypassStencilValue);
	TextComponent.SetCustomDepthStencilWriteMask(ERendererStencilMask::ERSM_Default);
}

FRotator UChopItCameraFacingTextComponent::CalculateFacingRotation(
	const FVector& TextLocation,
	const FVector& CameraLocation,
	const bool bIncludePitch)
{
	FVector ToCamera = CameraLocation - TextLocation;
	if (!bIncludePitch) ToCamera.Z = 0.0f;
	if (ToCamera.IsNearlyZero()) return FRotator::ZeroRotator;
	FRotator Result = ToCamera.Rotation();
	Result.Roll = 0.0f;
	return Result;
}

void UChopItCameraFacingTextComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsVisible() || !GetWorld()) return;

	const APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	const APlayerCameraManager* CameraManager = Controller ? Controller->PlayerCameraManager : nullptr;
	if (!CameraManager) return;

	if (CameraManager->GetCameraLocation().Equals(GetComponentLocation())) return;
	SetWorldRotation(CalculateFacingRotation(
		GetComponentLocation(), CameraManager->GetCameraLocation(), bFaceCameraPitch));
}
