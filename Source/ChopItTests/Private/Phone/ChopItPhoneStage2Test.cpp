#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneCallComponent.h"
#include "Phone/ChopItPhoneCallConfig.h"
#include "Phone/ChopItPhoneStateComponent.h"
#include "Framework/ChopItPlayerController.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/DefaultPawn.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "TimerManager.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItPhoneStage2Test, "ChopIt.Phone.Stage2.CallCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhoneStage2Test::RunTest(const FString& Parameters)
{
	UChopItPhoneCallConfig* Config = LoadObject<UChopItPhoneCallConfig>(nullptr,
		TEXT("/Game/ChopIt/Phone/DA_PhoneCall.DA_PhoneCall"));
	if (!TestNotNull(TEXT("Call Data Asset"), Config)) return false;
	TestTrue(TEXT("Default call config is valid"), Config->IsValidConfig());
	TestTrue(TEXT("Ring audio fades out at a nearby distance"), Config->AudibleDistance <= 900.0f);
	const float OriginalMaximum = Config->MaxIntervalSeconds;
	Config->MaxIntervalSeconds = Config->MinIntervalSeconds - 0.1f;
	TestFalse(TEXT("Reversed interval is rejected"), Config->IsValidConfig());
	Config->MaxIntervalSeconds = OriginalMaximum;

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	AChopItMysteryPhone* Phone = World->SpawnActor<AChopItMysteryPhone>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Phone actor"), Phone)) return false;
	Phone->DispatchBeginPlay();
	UChopItPhoneCallComponent* Call = Phone->GetPhoneCall();
	if (!TestNotNull(TEXT("Phone call component"), Call) ||
		!TestNotNull(TEXT("First ring audio channel"), Call->GetRingAudio()) ||
		!TestNotNull(TEXT("Second ring audio channel"), Call->GetSecondaryRingAudio())) return false;
	TestTrue(TEXT("Next call scheduled"), Call->IsCallScheduled());
	TestTrue(TEXT("Ring sound is attached to phone"), Call->GetRingAudio()->GetAttachParent() == Phone->GetRootComponent());
	TestTrue(TEXT("Second ring channel is attached to phone"),
		Call->GetSecondaryRingAudio()->GetAttachParent() == Phone->GetRootComponent());
	TestTrue(TEXT("Audio and comic share the phone hearing radius"),
		FMath::IsNearlyEqual(Call->GetAudibleDistance(), Phone->GetHearingRange()));
	TestTrue(TEXT("Editor sphere shows the hearing radius"),
		Phone->GetHearingRangeGizmo() &&
		FMath::IsNearlyEqual(Phone->GetHearingRangeGizmo()->GetUnscaledSphereRadius(), Phone->GetHearingRange()));
	TestTrue(TEXT("Audio attenuation ends at the phone hearing radius"),
		FMath::IsNearlyEqual(Call->GetRingAudio()->AttenuationOverrides.AttenuationShapeExtents.X +
			Call->GetRingAudio()->AttenuationOverrides.FalloffDistance, Phone->GetHearingRange()));
	TestTrue(TEXT("3D volume attenuation enabled"), Call->GetRingAudio()->bOverrideAttenuation &&
		Call->GetRingAudio()->AttenuationOverrides.bAttenuate && Call->GetRingAudio()->AttenuationOverrides.bSpatialize);

	const FTransform RestingShell = Phone->GetBaseVisual()->GetRelativeTransform();
	const FTransform RestingReceiver = Phone->GetReceiverVisual()->GetRelativeTransform();
	TestTrue(TEXT("Manual call starts"), Call->StartCall());
	TestEqual(TEXT("Ringing state"), Phone->GetPhoneState()->GetState(), EChopItPhoneState::Ringing);
	TestEqual(TEXT("First bell beat is immediate"), Call->GetEmittedBeatCount(), 1);
	TestFalse(TEXT("Shell rocks while ringing"), Phone->GetBaseVisual()->GetRelativeTransform().Equals(RestingShell));
	TestFalse(TEXT("Receiver bounces while ringing"), Phone->GetReceiverVisual()->GetRelativeTransform().Equals(RestingReceiver));
	TestFalse(TEXT("Concurrent call rejected"), Call->StartCall());
	TestFalse(TEXT("Interval timer cleared during call"), Call->IsCallScheduled());
	// Timers created before a manager tick enter its pending queue first.
	World->GetTimerManager().Tick(0.0f);
	++GFrameCounter;
	for (int32 ExpectedBeat = 2; ExpectedBeat <= 6; ++ExpectedBeat)
	{
		const bool bSecondStrikeInPair = ExpectedBeat % 2 == 0;
		const float Step = bSecondStrikeInPair ? Config->RingStrikeSpacingSeconds
			: Config->RingPairSpacingSeconds - Config->RingStrikeSpacingSeconds;
		World->GetTimerManager().Tick(Step + 0.01f);
		TestEqual(*FString::Printf(TEXT("Bell strike %d of six"), ExpectedBeat), Call->GetEmittedBeatCount(), ExpectedBeat);
		World->GetTimerManager().Tick(0.0f);
		++GFrameCounter;
	}
	World->GetTimerManager().Tick(Config->RingDurationSeconds - 2.0f * Config->RingPairSpacingSeconds
		- Config->RingStrikeSpacingSeconds + 0.1f);
	TestEqual(TEXT("Duration ends call"), Phone->GetPhoneState()->GetState(), EChopItPhoneState::Available);
	TestTrue(TEXT("Shell returns to rest after ringing"), Phone->GetBaseVisual()->GetRelativeTransform().Equals(RestingShell));
	TestTrue(TEXT("Receiver returns to rest after ringing"), Phone->GetReceiverVisual()->GetRelativeTransform().Equals(RestingReceiver));
	TestTrue(TEXT("Another call scheduled"), Call->IsCallScheduled());

	TestTrue(TEXT("Second call starts"), Call->StartCall());
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APawn* Player = World->SpawnActor<ADefaultPawn>(ADefaultPawn::StaticClass(),
		FVector(100, 0, 0), FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("Phone interactor"), Player)) return false;
	AChopItPlayerController* Controller = World->SpawnActor<AChopItPlayerController>();
	if (!TestNotNull(TEXT("Player controller"), Controller)) return false;
	Controller->Possess(Player);
	FVector ListenerLocation, ListenerFront, ListenerRight;
	Controller->GetAudioListenerPosition(ListenerLocation, ListenerFront, ListenerRight);
	TestTrue(TEXT("Audio listener uses the character, not the camera"),
		ListenerLocation.Equals(Player->GetActorLocation() + FVector(0, 0, 55), 0.1f));
	TestTrue(TEXT("Sound attenuation uses the character"),
		Controller->GetAudioListenerAttenuationOverridePosition(ListenerLocation) &&
		ListenerLocation.Equals(Player->GetActorLocation() + FVector(0, 0, 55), 0.1f));
	TestTrue(TEXT("Answer stops ringing"), Phone->Interact_Implementation(Player));
	TestTrue(TEXT("Answer begins the reveal"),
		Phone->GetPhoneState()->GetState() == EChopItPhoneState::Transforming ||
		Phone->GetPhoneState()->GetState() == EChopItPhoneState::Dialogue);
	TestFalse(TEXT("Answer clears future calls"), Call->IsCallScheduled());
	TestFalse(TEXT("Answered phone cannot ring"), Call->StartCall());
	World->GetTimerManager().Tick(Config->RingDurationSeconds + Config->MaxIntervalSeconds + 1.0f);
	TestTrue(TEXT("No call after answer"),
		Phone->GetPhoneState()->GetState() == EChopItPhoneState::Transforming ||
		Phone->GetPhoneState()->GetState() == EChopItPhoneState::Dialogue);
	AChopItMysteryPhone* TimedPhone = World->SpawnActor<AChopItMysteryPhone>(FVector(1000, 0, 0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Timed phone"), TimedPhone)) return false;
	TimedPhone->DispatchBeginPlay();
	TestTrue(TEXT("First call uses a timer"), TimedPhone->GetPhoneCall()->IsCallScheduled());
	World->GetTimerManager().Tick(0.0f);
	++GFrameCounter;
	World->GetTimerManager().Tick(Config->FirstCallDelaySeconds + 0.1f);
	TestEqual(TEXT("First call rings after the short initial delay"),
		TimedPhone->GetPhoneState()->GetState(), EChopItPhoneState::Ringing);
	World->GetTimerManager().Tick(0.0f);
	++GFrameCounter;
	World->GetTimerManager().Tick(Config->RingDurationSeconds + 0.1f);
	TestEqual(TEXT("Three-second ring ends"),
		TimedPhone->GetPhoneState()->GetState(), EChopItPhoneState::Available);
	World->GetTimerManager().Tick(0.0f);
	++GFrameCounter;
	World->GetTimerManager().Tick(Config->MinIntervalSeconds - Config->RingDurationSeconds + 0.1f);
	TestEqual(TEXT("Next ring starts ten seconds after the previous start"),
		TimedPhone->GetPhoneState()->GetState(), EChopItPhoneState::Ringing);
	return true;
}
#endif
