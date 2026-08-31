#include "Economy/ChopItTetherReceiverComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UChopItTetherReceiverComponent::UChopItTetherReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UChopItTetherReceiverComponent::SetTetherState(
	const FVector& InGuidePoint,
	const float InTensionAlpha,
	const bool bInHardLimit,
	const float InPullAcceleration,
	const float InPullDamping)
{
	GuidePoint = InGuidePoint;
	TensionAlpha = FMath::Clamp(InTensionAlpha, 0.0f, 1.0f);
	bHardLimit = bInHardLimit;
	PullAcceleration = FMath::Max(0.0f, InPullAcceleration);
	PullDamping = FMath::Max(0.0f, InPullDamping);
	bHasTetherState = true;
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
	Outward.Z = 0.0f;
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
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!bHasTetherState || !Movement)
	{
		return;
	}

	const FVector Outward = GetOutwardDirection();
	if (Outward.IsNearlyZero())
	{
		return;
	}
	const float OutwardSpeed = FVector::DotProduct(Movement->Velocity, Outward);
	if (bHardLimit && OutwardSpeed > 0.0f)
	{
		Movement->Velocity -= Outward * OutwardSpeed;
	}
	if (TensionAlpha > 0.0f)
	{
		const float DampedAcceleration = PullAcceleration * TensionAlpha
			+ FMath::Max(0.0f, OutwardSpeed) * PullDamping * TensionAlpha;
		Movement->AddForce(-Outward * DampedAcceleration * Movement->Mass);
	}
}
