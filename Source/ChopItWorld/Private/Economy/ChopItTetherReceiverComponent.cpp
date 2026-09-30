#include "Economy/ChopItTetherReceiverComponent.h"
#include "Economy/ChopItQuotaMachine.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UChopItTetherReceiverComponent::UChopItTetherReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UChopItTetherReceiverComponent::SetTetherState(
	const FVector& InGuidePoint,
	const float InTensionAlpha,
	const bool bInHardLimit)
{
	GuidePoint = InGuidePoint;
	TensionAlpha = FMath::Clamp(InTensionAlpha, 0.0f, 1.0f);
	bHardLimit = bInHardLimit;
	bHasTetherState = true;
}

void UChopItTetherReceiverComponent::BindMachine(AChopItQuotaMachine* InMachine)
{
	if (Machine.IsValid()) Machine->RemoveTickPrerequisiteComponent(this);
	Machine = InMachine;
	// Use the current Chaos body poses. Solving before physics left a falling
	// tree inside the rendered chain until the next frame. Forces produced here
	// are accumulated for the next rigid-body step, and presentation follows us.
	SetTickGroup(TG_PostPhysics);
	if (InMachine) InMachine->AddTickPrerequisiteComponent(this);
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (InMachine) AddTickPrerequisiteComponent(Character->GetCharacterMovement());
		else RemoveTickPrerequisiteComponent(Character->GetCharacterMovement());
	}
}

void UChopItTetherReceiverComponent::ClearTetherState()
{
	TensionAlpha = 0.0f;
	bHardLimit = false;
	bHasTetherState = false;
}

FVector UChopItTetherReceiverComponent::GetOutwardDirection() const
{
	const AActor* Owner = GetOwner();
	if (!bHasTetherState || !Owner)
	{
		return FVector::ZeroVector;
	}
	FVector Outward = Owner->GetActorLocation() - GuidePoint;
	return Outward.GetSafeNormal();
}

FVector UChopItTetherReceiverComponent::ConstrainMovementDirection(const FVector& WorldDirection) const
{
	if (!bHardLimit)
	{
		return WorldDirection;
	}
	const FVector Outward = GetOutwardDirection();
	const float OutwardAmount = FVector::DotProduct(WorldDirection, Outward);
	return OutwardAmount > 0.0f ? WorldDirection - Outward * OutwardAmount : WorldDirection;
}

void UChopItTetherReceiverComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Machine.IsValid()) Machine->AdvancePlayerChain(DeltaTime);
}
