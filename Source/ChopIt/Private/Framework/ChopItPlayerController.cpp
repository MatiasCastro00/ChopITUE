#include "Framework/ChopItPlayerController.h"

#include "ChopItLogChannels.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Framework/ChopItPlayerState.h"
#include "Progression/ChopItUpgradeOfferComponent.h"
#include "Shop/ChopItShopComponent.h"
#include "Pacts/ChopItPactComponent.h"
#include "Player/ChopItCharacter.h"
#include "Framework/ChopItGameState.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Camera/ChopItCameraDirectorSubsystem.h"
#include "Dialogue/ChopItDialogueSubsystem.h"
#include "Dialogue/ChopItDialogueAssets.h"
#include "Dialogue/ChopItDialogueStageCharacter.h"
#include "Camera/ChopItCameraAnchor.h"
#include "Economy/ChopItQuotaComponent.h"
#include "Economy/ChopItQuotaMachine.h"
#include "Economy/ChopItDeliveryZone.h"
#include "Economy/ChopItWoodGrantZone.h"
#include "Engine/Texture2D.h"
#include "InputAction.h"
#include "EngineUtils.h"
#include "Framework/ChopItPlayerCameraManager.h"
#include "TimerManager.h"
#include "UI/ChopItHUD.h"

AChopItPlayerController::AChopItPlayerController()
{
	PlayerCameraManagerClass = AChopItPlayerCameraManager::StaticClass();
	// Modal dialogue pauses the world, but camera framing still needs the controller's
	// full update path. The base class otherwise performs input-only pause ticks and
	// never refreshes PlayerCameraManager from the dialogue camera component.
	bShouldPerformFullTickWhenPaused = true;
}

void AChopItPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			const UInputMappingContext* GameplayContext = LoadObject<UInputMappingContext>(
				nullptr,
				TEXT("/Game/ChopIt/Input/IMC_Gameplay.IMC_Gameplay"));

			if (GameplayContext)
			{
				InputSubsystem->AddMappingContext(GameplayContext, 0);
			}
		}
	}

	// The authored introduction belongs to the real base match, not automation maps.
	if (IsLocalController() && GetWorld() && GetWorld()->GetMapName().Contains(TEXT("L_Startup")))
	{
		GetWorld()->GetTimerManager().SetTimer(MatchIntroTimer, this, &AChopItPlayerController::TryStartMatchIntro, 0.25f, true);
	}
}

AChopItCameraAnchor* AChopItPlayerController::SpawnIntroCameraAnchor(
	const FName Name,
	AActor* Subject,
	const FVector& Offset,
	const FVector& SubjectFocusOffset) const
{
	if (!GetWorld() || !Subject) return nullptr;
	FActorSpawnParameters Parameters;
	Parameters.Name = Name;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector SubjectFocus = Subject->GetActorLocation() + SubjectFocusOffset;
	const FVector Location = Subject->GetActorLocation() + Offset;
	AChopItCameraAnchor* Anchor = GetWorld()->SpawnActor<AChopItCameraAnchor>(Location, (SubjectFocus - Location).Rotation(), Parameters);
	if (Anchor)
	{
		Anchor->DefaultSubject = Subject;
		Anchor->SubjectFocusOffset = SubjectFocusOffset;
		Anchor->bUseActorLocationForSubject = true;
	}
	return Anchor;
}

void AChopItPlayerController::TryStartMatchIntro()
{
	if (bMatchIntroAttempted || !GetWorld() || !GetPawn() || !GetLocalPlayer()) return;
	AChopItGameState* State = GetWorld()->GetGameState<AChopItGameState>();
	UChopItDialogueSubsystem* Dialogue = GetLocalPlayer()->GetSubsystem<UChopItDialogueSubsystem>();
	UChopItDialogueSequence* Sequence = LoadObject<UChopItDialogueSequence>(
		nullptr,
		TEXT("/Game/ChopIt/Dialogue/Sequences/DA_Dialogue_MatchIntro.DA_Dialogue_MatchIntro"));
	if (!State || !State->GetQuotaComponent() || State->GetQuotaComponent()->GetTarget() <= 0 || !Dialogue || !Sequence) return;

	bMatchIntroAttempted = true;
	GetWorld()->GetTimerManager().ClearTimer(MatchIntroTimer);

	AChopItQuotaMachine* Oven = nullptr;
	for (TActorIterator<AChopItQuotaMachine> It(GetWorld()); It; ++It) { Oven = *It; break; }
	if (!Oven)
	{
		UClass* OvenClass = LoadClass<AChopItQuotaMachine>(
			nullptr,
			TEXT("/Game/ChopIt/World/Economy/BP_QuotaMachine.BP_QuotaMachine_C"));
		Oven = GetWorld()->SpawnActor<AChopItQuotaMachine>(
			OvenClass ? OvenClass : AChopItQuotaMachine::StaticClass(),
			FVector(180.0f, -480.0f, 0.0f),
			FRotator::ZeroRotator);
	}
	if (!Oven) return;
	EnsureStartupDeliveryActors(Oven);

	const FVector OvenLocation = Oven->GetActorLocation();
	for (TActorIterator<AChopItDialogueStageCharacter> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("Dialogue.Death")) || It->GetFName() == TEXT("Death_NPC"))
		{
			IntroDeath = *It;
			break;
		}
	}
	if (!IntroDeath)
	{
		// Compatibility fallback for older maps that have not run the dialogue bootstrap yet.
		IntroDeath = GetWorld()->SpawnActor<AChopItDialogueStageCharacter>(OvenLocation + FVector(-70.0f, 320.0f, 0.0f), FRotator::ZeroRotator);
	}
	IntroOvenPortrait = GetWorld()->SpawnActor<AChopItDialogueStageCharacter>(OvenLocation + FVector(0.0f, 0.0f, 155.0f), FRotator::ZeroRotator);
	if (IntroDeath) IntroDeath->ConfigurePillMarker(FText::FromString(TEXT("DEATH")));
	if (IntroOvenPortrait) IntroOvenPortrait->Configure(LoadObject<UTexture2D>(nullptr, TEXT("/Game/ChopIt/Dialogue/Portraits/Oven_Hungry.Oven_Hungry")), 250.0f);

	// The pill marker is much narrower than the old illustrated stand-in. Give it a
	// genuinely wider, higher establishing close-up and compose it above the UI.
	AChopItCameraAnchor* DeathCamera = SpawnIntroCameraAnchor(
		TEXT("IntroDeathCamera"), IntroDeath, FVector(760.0f, 420.0f, 390.0f), FVector(0.0f, 0.0f, 55.0f));
	// A distinct close shot for HUNGER: closer than the preceding wide shot and
	// focused on the machine origin, never on the rope/chain component bounds.
	AChopItCameraAnchor* OvenCamera = SpawnIntroCameraAnchor(
		TEXT("IntroOvenCamera"), Oven, FVector(360.0f, -300.0f, 220.0f), FVector(0.0f, 0.0f, 45.0f));

	// Build the protagonist shot from the real runtime positions. The camera stays
	// on the hub side of the pawn instead of blindly moving toward the map edge.
	FVector ToHub = OvenLocation - GetPawn()->GetActorLocation();
	ToHub.Z = 0.0f;
	ToHub = ToHub.GetSafeNormal(UE_SMALL_NUMBER, FVector(-1.0f, 0.0f, 0.0f));
	const FVector HubSide = ToHub * 420.0f - FVector::CrossProduct(FVector::UpVector, ToHub) * 260.0f;
	AChopItCameraAnchor* PlayerCamera = SpawnIntroCameraAnchor(
		TEXT("IntroPlayerCamera"), GetPawn(), HubSide + FVector(0.0f, 0.0f, 300.0f), FVector(0.0f, 0.0f, -60.0f));
	AChopItCameraAnchor* WideCamera = SpawnIntroCameraAnchor(
		TEXT("IntroWideCamera"), Oven, FVector(760.0f, 460.0f, 420.0f), FVector(0.0f, 0.0f, 100.0f));
	for (AChopItCameraAnchor* Anchor : {DeathCamera, OvenCamera, PlayerCamera, WideCamera}) if (Anchor) IntroCameraAnchors.Add(Anchor);

	FChopItDialogueContext Context;
	Context.Arguments.Add({TEXT("Quota"), FText::AsNumber(State->GetQuotaComponent()->GetTarget())});
	Context.Bindings.Add({TEXT("Death"), IntroDeath});
	Context.Bindings.Add({TEXT("Oven"), Oven});
	Context.Bindings.Add({TEXT("Player"), GetPawn()});
	Context.Bindings.Add({TEXT("DeathCamera"), DeathCamera});
	Context.Bindings.Add({TEXT("OvenCamera"), OvenCamera});
	Context.Bindings.Add({TEXT("PlayerCamera"), PlayerCamera});
	Context.Bindings.Add({TEXT("WideCamera"), WideCamera});

	Dialogue->OnDialogueEvent.AddUniqueDynamic(this, &AChopItPlayerController::HandleIntroDialogueEvent);
	Dialogue->OnDialogueEnded.AddUniqueDynamic(this, &AChopItPlayerController::HandleIntroDialogueEnded);
	const FChopItDialogueHandle IntroHandle = Dialogue->StartDialogue(Sequence, Context, EChopItDialogueStartPolicy::RejectIfBusy);
	if (!IntroHandle.IsValid())
	{
		UE_LOG(LogChopIt, Warning, TEXT("Base match dialogue introduction could not start."));
		HandleIntroDialogueEnded({}, EChopItDialogueEndReason::InvalidData);
	}
	else
	{
		UE_LOG(LogChopIt, Display, TEXT("Base match dialogue introduction started: quota=%d, cameras=%d."),
			State->GetQuotaComponent()->GetTarget(), IntroCameraAnchors.Num());
	}
}

void AChopItPlayerController::EnsureStartupDeliveryActors(AChopItQuotaMachine* Oven)
{
	if (!GetWorld() || !Oven) return;

	AChopItDeliveryZone* Delivery = nullptr;
	for (TActorIterator<AChopItDeliveryZone> It(GetWorld()); It; ++It)
	{
		Delivery = *It;
		break;
	}
	if (!Delivery)
	{
		FActorSpawnParameters Parameters;
		Parameters.Name = TEXT("StartupQuotaDeliveryZone");
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Delivery = GetWorld()->SpawnActor<AChopItDeliveryZone>(
			Oven->GetActorLocation() + FVector(340.0f, 220.0f, 15.0f),
			FRotator::ZeroRotator,
			Parameters);
	}
	if (Delivery) Delivery->SetTargetMachine(Oven);

	AChopItWoodGrantZone* GrantZone = nullptr;
	for (TActorIterator<AChopItWoodGrantZone> It(GetWorld()); It; ++It)
	{
		GrantZone = *It;
		break;
	}
	if (!GrantZone)
	{
		FActorSpawnParameters Parameters;
		Parameters.Name = TEXT("StartupWoodGrantZone_200");
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GrantZone = GetWorld()->SpawnActor<AChopItWoodGrantZone>(
			Oven->GetActorLocation() + FVector(720.0f, 860.0f, 15.0f),
			FRotator::ZeroRotator,
			Parameters);
	}

	UE_LOG(LogChopIt, Display, TEXT("Startup delivery actors ready: delivery=%s grant=%s."),
		Delivery ? *Delivery->GetName() : TEXT("missing"),
		GrantZone ? *GrantZone->GetName() : TEXT("missing"));
}

void AChopItPlayerController::HandleIntroDialogueEvent(const FGameplayTag EventTag, const FChopItDialogueEventPayload Payload)
{
	if (!Payload.Sequence || Payload.Sequence->DialogueId != TEXT("MatchIntro")) return;
	const FName TagName = EventTag.GetTagName();
	if (TagName == TEXT("Dialogue.Event.DeathGesture") && IntroDeath) IntroDeath->PlayReaction(TEXT("Gesture"));
	else if (TagName == TEXT("Dialogue.Event.OvenRoar") && IntroOvenPortrait) IntroOvenPortrait->PlayReaction(TEXT("Roar"));
	else if (TagName == TEXT("Dialogue.Event.ChainPull") && IntroOvenPortrait) IntroOvenPortrait->PlayReaction(TEXT("Roar"));
	else if (TagName == TEXT("Dialogue.Event.DeathVanish") && IntroDeath) IntroDeath->BeginExit();
	else if (TagName == TEXT("Dialogue.Event.QuestStart"))
	{
		if (AChopItHUD* ChopItHUD = GetHUD<AChopItHUD>()) ChopItHUD->RevealMissionTracker();
	}
	else if (TagName == TEXT("Dialogue.Event.ChainReveal"))
	{
		if (IntroDeath) IntroDeath->PlayReaction(TEXT("Gesture"));
		if (IntroOvenPortrait) IntroOvenPortrait->PlayReaction(TEXT("Roar"));
	}
}

void AChopItPlayerController::HandleIntroDialogueEnded(const FChopItDialogueHandle, const EChopItDialogueEndReason)
{
	if (AChopItHUD* ChopItHUD = GetHUD<AChopItHUD>()) ChopItHUD->RevealMissionTracker();
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UChopItDialogueSubsystem* Dialogue = LP->GetSubsystem<UChopItDialogueSubsystem>())
		{
			Dialogue->OnDialogueEvent.RemoveDynamic(this, &AChopItPlayerController::HandleIntroDialogueEvent);
			Dialogue->OnDialogueEnded.RemoveDynamic(this, &AChopItPlayerController::HandleIntroDialogueEnded);
		}
	}
	if (IntroDeath) IntroDeath->BeginExit();
	if (IntroOvenPortrait) IntroOvenPortrait->BeginExit();
	for (AActor* Anchor : IntroCameraAnchors) if (IsValid(Anchor)) Anchor->Destroy();
	IntroCameraAnchors.Reset();
}

void AChopItPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent)
	{
		return;
	}
	FInputKeyBinding& One = InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AChopItPlayerController::SelectUpgradeOne);
	One.bExecuteWhenPaused = true;
	FInputKeyBinding& Two = InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AChopItPlayerController::SelectUpgradeTwo);
	Two.bExecuteWhenPaused = true;
	FInputKeyBinding& Three = InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AChopItPlayerController::SelectUpgradeThree);
	Three.bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AChopItPlayerController::CloseShop);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		DialogueAdvanceAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/ChopIt/Input/IA_DialogueAdvance.IA_DialogueAdvance"));
		DialogueNextChoiceAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/ChopIt/Input/IA_DialogueNextChoice.IA_DialogueNextChoice"));
		DialoguePreviousChoiceAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/ChopIt/Input/IA_DialoguePreviousChoice.IA_DialoguePreviousChoice"));
		DialogueCancelAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/ChopIt/Input/IA_DialogueCancel.IA_DialogueCancel"));
		if (DialogueAdvanceAction) EnhancedInput->BindAction(DialogueAdvanceAction, ETriggerEvent::Started, this, &AChopItPlayerController::DialogueAdvance);
		if (DialogueNextChoiceAction) EnhancedInput->BindAction(DialogueNextChoiceAction, ETriggerEvent::Started, this, &AChopItPlayerController::DialogueNextChoice);
		if (DialoguePreviousChoiceAction) EnhancedInput->BindAction(DialoguePreviousChoiceAction, ETriggerEvent::Started, this, &AChopItPlayerController::DialoguePreviousChoice);
		if (DialogueCancelAction) EnhancedInput->BindAction(DialogueCancelAction, ETriggerEvent::Started, this, &AChopItPlayerController::DialogueCancel);
	}
}

void AChopItPlayerController::DialogueAdvance()
{
	if (ULocalPlayer* LP = GetLocalPlayer()) if (UChopItDialogueSubsystem* Dialogue = LP->GetSubsystem<UChopItDialogueSubsystem>()) Dialogue->AdvanceOrComplete();
}

void AChopItPlayerController::DialogueNextChoice()
{
	if (ULocalPlayer* LP = GetLocalPlayer()) if (UChopItDialogueSubsystem* Dialogue = LP->GetSubsystem<UChopItDialogueSubsystem>()) Dialogue->MoveChoice(1);
}

void AChopItPlayerController::DialoguePreviousChoice()
{
	if (ULocalPlayer* LP = GetLocalPlayer()) if (UChopItDialogueSubsystem* Dialogue = LP->GetSubsystem<UChopItDialogueSubsystem>()) Dialogue->MoveChoice(-1);
}

void AChopItPlayerController::DialogueCancel()
{
	if (ULocalPlayer* LP = GetLocalPlayer()) if (UChopItDialogueSubsystem* Dialogue = LP->GetSubsystem<UChopItDialogueSubsystem>()) Dialogue->CancelDialogue();
}

void AChopItPlayerController::SelectUpgradeOne()
{
	SelectUpgrade(0);
}

void AChopItPlayerController::SelectUpgradeTwo()
{
	SelectUpgrade(1);
}

void AChopItPlayerController::SelectUpgradeThree()
{
	SelectUpgrade(2);
}

void AChopItPlayerController::SelectUpgrade(const int32 Index)
{
	if (ULocalPlayer* LP = GetLocalPlayer()) if (const UChopItCameraDirectorSubsystem* Camera = LP->GetSubsystem<UChopItCameraDirectorSubsystem>(); Camera && Camera->IsInputLocked(EChopItCameraInputLock::Actions)) return;
	if (const AChopItGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChopItGameState>() : nullptr)
	{
		if (UChopItCycleStateMachineComponent* Cycle = GameState->GetCycleStateMachine();
			Cycle && Cycle->GetCurrentPhase() == EChopItCyclePhase::Victory)
		{
			if (Index == 0) { Cycle->RequestVictoryRetirement(); }
			else if (Index == 1) { Cycle->RequestInfiniteMode(); }
			return;
		}
	}
	if (const AChopItPlayerState* State = Cast<AChopItPlayerState>(PlayerState))
	{
		if (UChopItUpgradeOfferComponent* Offers = State->GetUpgradeOfferComponent())
		{
			if (Offers->HasActiveOffer())
			{
				Offers->SelectOffer(Index);
				return;
			}
		}
		if (UChopItShopComponent* Shop = State->GetShopComponent())
		{
			if (UChopItPactComponent* Pacts=State->GetPactComponent(); Pacts && Pacts->HasActiveOffer())
			{
				if (AChopItCharacter* ChopItCharacter=Cast<AChopItCharacter>(GetPawn())) Pacts->SelectOffer(Index,ChopItCharacter->GetCombatStatsComponent());
				return;
			}
			Shop->SelectOffer(Index, GetPawn());
		}
	}
}

void AChopItPlayerController::CloseShop()
{
	if (ULocalPlayer* LP = GetLocalPlayer()) if (const UChopItCameraDirectorSubsystem* Camera = LP->GetSubsystem<UChopItCameraDirectorSubsystem>(); Camera && Camera->IsInputLocked(EChopItCameraInputLock::Actions)) return;
	if (const AChopItPlayerState* State = Cast<AChopItPlayerState>(PlayerState))
	{
		if (UChopItShopComponent* Shop = State->GetShopComponent()) { Shop->CloseShop(); }
	}
}
