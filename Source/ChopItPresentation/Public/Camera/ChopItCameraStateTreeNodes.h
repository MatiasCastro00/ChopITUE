#pragma once

#include "CoreMinimal.h"
#include "Camera/ChopItCameraTypes.h"
#include "Directors/CameraDirectorStateTreeSchema.h"
#include "StateTreeExecutionTypes.h"
#include "StateTreeTypes.h"
#include "ChopItCameraStateTreeNodes.generated.h"

class UCameraRigAsset;
struct FStateTreeExecutionContext;
struct FStateTreeLinker;

USTRUCT()
struct CHOPITPRESENTATION_API FChopItActivateCameraRigTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Camera", meta=(UseCameraDirectorRigPicker=true))
	TObjectPtr<UCameraRigAsset> CameraRig;
};

/**
 * ChopIt-owned equivalent of Gameplay Cameras' built-in activation task.
 * Keeping this node in our module protects runtime code from UE 5.8's
 * experimental MinimalAPI boundary.
 */
USTRUCT(meta=(DisplayName="Activate ChopIt Camera Rig", Category="ChopIt|Camera"))
struct CHOPITPRESENTATION_API FChopItActivateCameraRigTask : public FGameplayCamerasStateTreeTask
{
	GENERATED_BODY()
	using FInstanceDataType = FChopItActivateCameraRigTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;

private:
	bool ActivateRig(FStateTreeExecutionContext& Context) const;

	TStateTreeExternalDataHandle<FCameraDirectorStateTreeEvaluationData> EvaluationDataHandle;
};

/** Selects a camera state from the public mode exposed by the ChopIt component. */
USTRUCT()
struct CHOPITPRESENTATION_API FChopItCameraModeConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Camera")
	EChopItCameraMode ExpectedMode = EChopItCameraMode::GameplayOrbit;
};

USTRUCT(meta=(DisplayName="ChopIt Camera Mode", Category="ChopIt|Camera"))
struct CHOPITPRESENTATION_API FChopItCameraModeCondition : public FGameplayCamerasStateTreeCondition
{
	GENERATED_BODY()
	using FInstanceDataType = FChopItCameraModeConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
