#include "Camera/ChopItCameraStateTreeNodes.h"

#include "Camera/ChopItCameraComponent.h"
#include "Core/CameraRigAsset.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FChopItActivateCameraRigTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EvaluationDataHandle);
	return true;
}

EStateTreeRunStatus FChopItActivateCameraRigTask::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	return ActivateRig(Context) ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FChopItActivateCameraRigTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	ActivateRig(Context);
	return EStateTreeRunStatus::Running;
}

bool FChopItActivateCameraRigTask::ActivateRig(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.CameraRig)
	{
		return false;
	}

	FCameraDirectorStateTreeEvaluationData& EvaluationData = Context.GetExternalData(EvaluationDataHandle);
	EvaluationData.ActiveCameraRigs.Add(InstanceData.CameraRig);
	return true;
}

bool FChopItCameraModeCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	// FStateTreeContextDataNames::ContextOwner is not exported by the
	// experimental GameplayCameras module in UE 5.8. Its schema contract is the
	// stable name "ContextOwner", so keep the boundary local to this adapter.
	static const FName ContextOwnerName(TEXT("ContextOwner"));
	FStateTreeDataView OwnerView = Context.GetContextDataByName(ContextOwnerName);
	const UChopItCameraComponent* CameraComponent = Cast<UChopItCameraComponent>(OwnerView.GetMutablePtr<UObject>());
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return CameraComponent && CameraComponent->GetActiveMode() == InstanceData.ExpectedMode;
}
