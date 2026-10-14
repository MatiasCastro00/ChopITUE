#include "Phone/ChopItPhoneRingMotionComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneCallComponent.h"

UChopItPhoneRingMotionComponent::UChopItPhoneRingMotionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItPhoneRingMotionComponent::BeginPlay()
{
	Super::BeginPlay();
	Phone = Cast<AChopItMysteryPhone>(GetOwner());
	if (!Phone.IsValid() || !Phone->GetBaseVisual() || !Phone->GetReceiverVisual()) return;
	Call = Phone->GetPhoneCall();
	OriginalShell = Phone->GetBaseVisual()->GetRelativeTransform();
	OriginalReceiver = Phone->GetReceiverVisual()->GetRelativeTransform();
	if (Call.IsValid())
	{
		Call->OnRingStarted.AddDynamic(this, &UChopItPhoneRingMotionComponent::HandleRingStarted);
		Call->OnRingStopped.AddDynamic(this, &UChopItPhoneRingMotionComponent::HandleRingStopped);
		Call->OnRingBeat.AddDynamic(this, &UChopItPhoneRingMotionComponent::HandleRingBeat);
	}
}

void UChopItPhoneRingMotionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Call.IsValid())
	{
		Call->OnRingStarted.RemoveDynamic(this, &UChopItPhoneRingMotionComponent::HandleRingStarted);
		Call->OnRingStopped.RemoveDynamic(this, &UChopItPhoneRingMotionComponent::HandleRingStopped);
		Call->OnRingBeat.RemoveDynamic(this, &UChopItPhoneRingMotionComponent::HandleRingBeat);
	}
	HandleRingStopped();
	Super::EndPlay(EndPlayReason);
}

void UChopItPhoneRingMotionComponent::HandleRingStarted()
{
	if (!Phone.IsValid()) return;
	Elapsed = 0.0f;
	RestoreVisuals();
}

void UChopItPhoneRingMotionComponent::HandleRingBeat(int32 BeatIndex)
{
	if (!Phone.IsValid() || !GetWorld()) return;
	Elapsed = 0.0f;
	GetWorld()->GetTimerManager().SetTimer(MotionTimer, this,
		&UChopItPhoneRingMotionComponent::UpdateMotion, FMath::Max(0.01f, UpdatePeriod), true);
	UpdateMotion();
}

void UChopItPhoneRingMotionComponent::HandleRingStopped()
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(MotionTimer);
	RestoreVisuals();
}

void UChopItPhoneRingMotionComponent::UpdateMotion()
{
	if (!Phone.IsValid()) { HandleRingStopped(); return; }
	Elapsed += FMath::Max(0.01f, UpdatePeriod);
	const float Phase = FMath::Clamp(Elapsed / FMath::Max(0.05f, BeatDurationSeconds), 0.0f, 1.0f);
	if (Phase >= 1.0f)
	{
		HandleRingStopped();
		return;
	}
	const float Wave = FMath::Sin(2.0f * PI * Phase) * FMath::Sin(PI * Phase);
	const float Bounce = FMath::Abs(Wave);
	if (UStaticMeshComponent* Shell = Phone->GetBaseVisual())
	{
		Shell->SetRelativeLocationAndRotation(
			OriginalShell.GetLocation() + FVector(0, 0, Bounce * ShellBobCm),
			OriginalShell.Rotator() + FRotator(0, Wave * ShellRockDegrees * 0.35f, Wave * ShellRockDegrees));
	}
	if (UStaticMeshComponent* Receiver = Phone->GetReceiverVisual())
	{
		Receiver->SetRelativeLocationAndRotation(
			OriginalReceiver.GetLocation() + FVector(Wave * 0.8f, 0, Bounce * ReceiverHopCm),
			OriginalReceiver.Rotator() + FRotator(Wave * ReceiverRockDegrees * 0.2f,
				-Wave * ReceiverRockDegrees * 0.3f, Wave * ReceiverRockDegrees));
	}
}

void UChopItPhoneRingMotionComponent::RestoreVisuals()
{
	if (!Phone.IsValid()) return;
	if (UStaticMeshComponent* Shell = Phone->GetBaseVisual()) Shell->SetRelativeTransform(OriginalShell);
	if (UStaticMeshComponent* Receiver = Phone->GetReceiverVisual()) Receiver->SetRelativeTransform(OriginalReceiver);
}
