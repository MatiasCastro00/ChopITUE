#pragma once

#include "Components/TextRenderComponent.h"
#include "ChopItCameraFacingTextComponent.generated.h"

/** TextRender that continuously billboards toward the final local player camera. */
UCLASS(ClassGroup = (ChopIt), BlueprintType, meta = (BlueprintSpawnableComponent))
class CHOPITCORE_API UChopItCameraFacingTextComponent : public UTextRenderComponent
{
	GENERATED_BODY()

public:
	/** Reserved stencil used by the PSX post-process to preserve readable world-space text. */
	static constexpr int32 PsxBypassStencilValue = 240;

	UChopItCameraFacingTextComponent();
	static void ConfigurePsxBypass(UTextRenderComponent& TextComponent);
	static FRotator CalculateFacingRotation(
		const FVector& TextLocation,
		const FVector& CameraLocation,
		bool bIncludePitch);
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Keeps labels upright for flat cameras; enabled gives a true spherical billboard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChopIt|Text")
	bool bFaceCameraPitch = true;
};
