#include "Dialogue/ChopItDialogueSubsystem.h"

#include "Camera/ChopItCameraAnchor.h"
#include "Camera/ChopItCameraDirectorSubsystem.h"
#include "Camera/ChopItCameraUserSettings.h"
#include "Camera/ChopItCameraCue.h"
#include "ChopItLogChannels.h"
#include "Components/AudioComponent.h"
#include "Containers/Ticker.h"
#include "Core/CameraShakeAsset.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UI/ChopItDialogueWidget.h"

void UChopItDialogueSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UChopItDialogueSubsystem::Tick));
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UChopItDialogueSubsystem::HandleWorldCleanup);
}

void UChopItDialogueSubsystem::Deinitialize()
{
	if (TickerHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	if (WorldCleanupHandle.IsValid()) FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	FinishActive(EChopItDialogueEndReason::Stopped, false);
	Queue.Reset();
	Super::Deinitialize();
}

void UChopItDialogueSubsystem::HandleWorldCleanup(UWorld* World, const bool, const bool)
{
	APlayerController* PC = ResolvePlayerController();
	if (IsDialogueActive() && (!PC || PC->GetWorld() == World))
	{
		FinishActive(EChopItDialogueEndReason::Stopped, false);
		Queue.Reset();
	}
}

APlayerController* UChopItDialogueSubsystem::ResolvePlayerController() const
{
	return GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
}

FChopItDialogueHandle UChopItDialogueSubsystem::StartDialogue(UChopItDialogueSequence* Sequence, const FChopItDialogueContext& Context, const EChopItDialogueStartPolicy Policy)
{
	if (!IsValid(Sequence)) return {};
	FChopItPendingDialogueRequest Request;
	Request.Handle.Id = FGuid::NewGuid();
	Request.Sequence = Sequence;
	Request.Context = Context;
	if (IsDialogueActive())
	{
		if (Policy == EChopItDialogueStartPolicy::RejectIfBusy) return {};
		if (Policy == EChopItDialogueStartPolicy::Queue) { Queue.Add(Request); return Request.Handle; }
		FinishActive(EChopItDialogueEndReason::Replaced, false);
	}
	BeginRequest(Request);
	return Request.Handle;
}

void UChopItDialogueSubsystem::BeginRequest(const FChopItPendingDialogueRequest& Request)
{
	TArray<FText> Errors;
	if (!Request.Sequence || !Request.Sequence->ValidateSequence(Errors))
	{
		CurrentHandle = Request.Handle;
		OnDialogueEnded.Broadcast(CurrentHandle, EChopItDialogueEndReason::InvalidData);
		CurrentHandle.Invalidate();
		if (!Queue.IsEmpty()) { const FChopItPendingDialogueRequest Next = Queue[0]; Queue.RemoveAt(0); BeginRequest(Next); }
		return;
	}

	CurrentHandle = Request.Handle;
	CurrentSequence = Request.Sequence;
	CurrentContext = Request.Context;
	VisitCount = 0;
	APlayerController* PC = ResolvePlayerController();
	if (!PC)
	{
		FinishActive(EChopItDialogueEndReason::InvalidData);
		return;
	}

	Widget = CreateWidget<UChopItDialogueWidget>(PC, UChopItDialogueWidget::StaticClass());
	if (!Widget)
	{
		FinishActive(EChopItDialogueEndReason::InvalidData);
		return;
	}
	Widget->AddToPlayerScreen(1000);
	const UChopItCameraUserSettings* Settings = Cast<UChopItCameraUserSettings>(UGameUserSettings::GetGameUserSettings());
	Widget->ConfigureTheme(
		CurrentSequence->Theme,
		Settings && Settings->bReduceDialogueMotion,
		Settings ? Settings->DialogueUIScale : 1.0f);

	if (CurrentSequence->bPauseWorld && !PC->IsPaused()) bOwnedPause = PC->SetPause(true);
	SetDialogueInputEnabled(true);
	if (CurrentSequence->bBlockGameplayInput && GetLocalPlayer())
	{
		if (UChopItCameraDirectorSubsystem* Camera = GetLocalPlayer()->GetSubsystem<UChopItCameraDirectorSubsystem>())
		{
			InputLockHandle = Camera->PushInputLock(EChopItCameraInputLock::Camera | EChopItCameraInputLock::Movement | EChopItCameraInputLock::Actions);
		}
	}
	OnDialogueStarted.Broadcast(CurrentHandle, CurrentSequence);
	BroadcastEvent(CurrentSequence->StartEvent);
	EnterLine(CurrentSequence->EntryLineId, true);
}

void UChopItDialogueSubsystem::EnterLine(const FName LineId, const bool bFirstLine)
{
	if (++VisitCount > FMath::Max(1, CurrentSequence ? CurrentSequence->VisitLimit : 1))
	{
		FinishActive(EChopItDialogueEndReason::VisitLimit);
		return;
	}
	CurrentLine = CurrentSequence ? CurrentSequence->FindLine(LineId) : nullptr;
	if (!CurrentLine)
	{
		FinishActive(EChopItDialogueEndReason::InvalidData);
		return;
	}

	FFormatNamedArguments Arguments;
	for (const FChopItDialogueArgument& Argument : CurrentContext.Arguments) Arguments.Add(Argument.Name.ToString(), Argument.Value);
	const FText Formatted = Arguments.IsEmpty() ? CurrentLine->Text : FText::Format(CurrentLine->Text, Arguments);
	CurrentDocument = FChopItDialogueMarkup::Compile(Formatted.ToString());
	if (!CurrentDocument.bValid) UE_LOG(LogTemp, Warning, TEXT("Dialogue %s line %s markup fallback: %s"), *CurrentSequence->GetName(), *LineId.ToString(), *CurrentDocument.Error);

	RevealCount = 0;
	CueCursor = 0;
	SelectedChoice = 0;
	BlipCounter = 0;
	TimeUntilNextGlyph = 0.0f;
	AutoAdvanceElapsed = 0.0f;
	StateElapsed = 0.0f;
	bFirstLineTransition = bFirstLine;
	bEndingAfterExit = false;
	AvailableChoiceIndices.Reset();
	Widget->ShowLine(*CurrentLine, CurrentDocument);
	Widget->SetTransitionProgress(0.0f, false, bFirstLine);
	State = EChopItDialogueState::Entering;

	if (CurrentLine->Voice.ToSoftObjectPath().IsValid())
	{
		if (USoundBase* Voice = CurrentLine->Voice.LoadSynchronous()) VoiceComponent = UGameplayStatics::SpawnSound2D(this, Voice);
	}
	BroadcastEvent(CurrentLine->StartEvent);
	OnLineStarted.Broadcast(CurrentHandle, CurrentLine->LineId);
	ExecuteCameraAction(CurrentLine->CameraAction);
	ProcessCues(0, false);
}

bool UChopItDialogueSubsystem::Tick(const float DeltaSeconds)
{
	if (!IsDialogueActive()) return true;
	if (!ResolvePlayerController()) { FinishActive(EChopItDialogueEndReason::Stopped); return true; }
	StateElapsed += DeltaSeconds;
	const UChopItDialogueTheme* Theme = CurrentSequence ? CurrentSequence->Theme : nullptr;
	const float EnterDuration = Theme ? Theme->EnterDuration : 0.18f;
	const float ExitDuration = Theme ? Theme->ExitDuration : 0.22f;
	UpdateSustainedCameraShake(DeltaSeconds);

	if (State == EChopItDialogueState::Entering)
	{
		const float Alpha = EnterDuration <= 0.0f ? 1.0f : StateElapsed / EnterDuration;
		Widget->SetTransitionProgress(Alpha, false, bFirstLineTransition);
		if (Alpha >= 1.0f)
		{
			Widget->ResetTransition();
			State = EChopItDialogueState::Revealing;
			StateElapsed = 0.0f;
			const UChopItCameraUserSettings* Settings = Cast<UChopItCameraUserSettings>(UGameUserSettings::GetGameUserSettings());
			if (Settings && Settings->bInstantDialogueText) FinishReveal(true);
		}
	}
	else if (State == EChopItDialogueState::Revealing)
	{
		TimeUntilNextGlyph -= DeltaSeconds;
		const UChopItCameraUserSettings* Settings = Cast<UChopItCameraUserSettings>(UGameUserSettings::GetGameUserSettings());
		const float UserSpeed = Settings ? Settings->DialogueTextSpeed : 1.0f;
		while (TimeUntilNextGlyph <= 0.0f && RevealCount < CurrentDocument.Glyphs.Num())
		{
			const FChopItDialogueGlyph& Glyph = CurrentDocument.Glyphs[RevealCount++];
			Widget->SetRevealCount(RevealCount);
			ProcessCues(RevealCount, false);
			const float CharactersPerSecond = 35.0f * FMath::Max(0.05f, CurrentLine->TypewriterSpeed) * FMath::Max(0.25f, UserSpeed) * FMath::Max(0.05f, Glyph.Style.SpeedScale);
			TimeUntilNextGlyph += 1.0f / CharactersPerSecond + Glyph.ExtraDelayAfter;
			const UChopItDialogueSpeakerDefinition* Speaker = CurrentLine->Speaker;
			const bool bHasVoice = VoiceComponent && VoiceComponent->IsPlaying();
			if (Speaker && (!bHasVoice || Speaker->bPlayBlipsUnderVoice) && !Glyph.Grapheme.TrimStartAndEnd().IsEmpty() && ++BlipCounter % FMath::Max(1, Speaker->BlipCadence) == 0)
			{
				if (USoundBase* Blip = Speaker->TypewriterBlip.LoadSynchronous()) UGameplayStatics::PlaySound2D(this, Blip, 0.35f, FMath::FRandRange(Speaker->MinBlipPitch, Speaker->MaxBlipPitch));
			}
		}
		if (RevealCount >= CurrentDocument.Glyphs.Num()) FinishReveal(false);
	}
	else if (State == EChopItDialogueState::AwaitingAdvance && CurrentLine && CurrentLine->bAutoAdvance)
	{
		if (!VoiceComponent || !VoiceComponent->IsPlaying()) AutoAdvanceElapsed += DeltaSeconds;
		if (AutoAdvanceElapsed >= CurrentLine->AutoAdvanceDelay) BeginAdvance(CurrentLine->NextLineId);
	}
	else if (State == EChopItDialogueState::Exiting)
	{
		const float Alpha = ExitDuration <= 0.0f ? 1.0f : StateElapsed / ExitDuration;
		Widget->SetTransitionProgress(Alpha, true, bEndingAfterExit);
		if (Alpha >= 1.0f)
		{
			if (bEndingAfterExit) FinishActive(PendingEndReason);
			else { ClearLineCameraActions(); EnterLine(PendingLineId, false); }
		}
	}
	return true;
}

void UChopItDialogueSubsystem::AdvanceOrComplete()
{
	if (State == EChopItDialogueState::Revealing) { FinishReveal(true); return; }
	if (State == EChopItDialogueState::ShowingChoices) { SelectChoice(); return; }
	if (State == EChopItDialogueState::AwaitingAdvance && CurrentLine) BeginAdvance(CurrentLine->NextLineId);
}

void UChopItDialogueSubsystem::FinishReveal(const bool bFastForward)
{
	if (bFastForward)
	{
		RevealCount = CurrentDocument.Glyphs.Num();
		Widget->SetRevealCount(RevealCount);
		ProcessCues(RevealCount, true);
	}
	if (!bSustainCameraShakeUntilLineEnd) StopSustainedCameraShake();
	RefreshChoices();
	if (!AvailableChoiceIndices.IsEmpty()) State = EChopItDialogueState::ShowingChoices;
	else State = EChopItDialogueState::AwaitingAdvance;
	StateElapsed = 0.0f;
	Widget->SetAwaitingAdvance(State == EChopItDialogueState::AwaitingAdvance);
}

void UChopItDialogueSubsystem::RefreshChoices()
{
	AvailableChoiceIndices.Reset();
	TArray<FChopItDialogueChoiceView> Views;
	if (CurrentLine)
	{
		for (int32 Index = 0; Index < CurrentLine->Choices.Num(); ++Index)
		{
			const FChopItDialogueChoice& Choice = CurrentLine->Choices[Index];
			if (!Choice.IsAvailable(CurrentContext.Tags)) continue;
			AvailableChoiceIndices.Add(Index);
			FFormatNamedArguments Arguments;
			for (const FChopItDialogueArgument& Argument : CurrentContext.Arguments)
			{
				Arguments.Add(Argument.Name.ToString(), Argument.Value);
			}
			Views.Add({Choice.ChoiceId, Arguments.IsEmpty() ? Choice.Text : FText::Format(Choice.Text, Arguments)});
		}
	}
	SelectedChoice = FMath::Clamp(SelectedChoice, 0, FMath::Max(0, Views.Num() - 1));
	Widget->SetChoices(Views, SelectedChoice);
}

void UChopItDialogueSubsystem::MoveChoice(const int32 Direction)
{
	if (State != EChopItDialogueState::ShowingChoices || AvailableChoiceIndices.IsEmpty()) return;
	SelectedChoice = (SelectedChoice + (Direction >= 0 ? 1 : -1) + AvailableChoiceIndices.Num()) % AvailableChoiceIndices.Num();
	Widget->SetSelectedChoice(SelectedChoice);
}

bool UChopItDialogueSubsystem::SelectChoice(const int32 VisibleChoiceIndex)
{
	if (State != EChopItDialogueState::ShowingChoices || !CurrentLine || AvailableChoiceIndices.IsEmpty()) return false;
	const int32 Visible = VisibleChoiceIndex == INDEX_NONE ? SelectedChoice : VisibleChoiceIndex;
	if (!AvailableChoiceIndices.IsValidIndex(Visible)) return false;
	const FChopItDialogueChoice& Choice = CurrentLine->Choices[AvailableChoiceIndices[Visible]];
	OnChoiceSelected.Broadcast(CurrentHandle, CurrentLine->LineId, Choice.ChoiceId);
	BroadcastEvent(Choice.EventTag, NAME_None, Choice.ChoiceId);
	BeginAdvance(Choice.NextLineId);
	return true;
}

void UChopItDialogueSubsystem::BeginAdvance(const FName NextLineId, const EChopItDialogueEndReason EndReason)
{
	if (!CurrentLine || State == EChopItDialogueState::Exiting) return;
	BroadcastEvent(CurrentLine->EndEvent);
	OnLineFinished.Broadcast(CurrentHandle, CurrentLine->LineId);
	PendingLineId = NextLineId;
	PendingEndReason = EndReason;
	bEndingAfterExit = NextLineId.IsNone();
	State = EChopItDialogueState::Exiting;
	StateElapsed = 0.0f;
	Widget->SetAwaitingAdvance(false);
}

bool UChopItDialogueSubsystem::CancelDialogue()
{
	if (!CurrentSequence || !CurrentSequence->bCanCancel || State == EChopItDialogueState::Exiting) return false;
	BeginAdvance(NAME_None, EChopItDialogueEndReason::Cancelled);
	return true;
}

bool UChopItDialogueSubsystem::StopDialogue(const FChopItDialogueHandle Handle)
{
	if (!Handle.IsValid()) return false;
	if (CurrentHandle == Handle) { FinishActive(EChopItDialogueEndReason::Stopped); return true; }
	const int32 Index = Queue.IndexOfByPredicate([&Handle](const FChopItPendingDialogueRequest& Request) { return Request.Handle == Handle; });
	if (Index == INDEX_NONE) return false;
	Queue.RemoveAt(Index);
	OnDialogueEnded.Broadcast(Handle, EChopItDialogueEndReason::Stopped);
	return true;
}

void UChopItDialogueSubsystem::ProcessCues(const int32 UpToGlyph, const bool bFastForward)
{
	while (CurrentDocument.Cues.IsValidIndex(CueCursor) && CurrentDocument.Cues[CueCursor].GlyphIndex <= UpToGlyph)
	{
		const FChopItDialogueMarkupCue& Cue = CurrentDocument.Cues[CueCursor++];
		if (!bFastForward || Cue.bFireOnFastForward) ExecuteCue(Cue);
		if (!bFastForward && Cue.PauseSeconds > 0.0f) TimeUntilNextGlyph += Cue.PauseSeconds;
	}
}

void UChopItDialogueSubsystem::ExecuteCue(const FChopItDialogueMarkupCue& Cue)
{
	if (Cue.EventTag.IsValid()) BroadcastEvent(Cue.EventTag, Cue.MarkerId, NAME_None, Cue.TargetBinding);
	if (!Cue.Face.IsNone() && Widget && CurrentLine) Widget->SetPortraitExpression(CurrentLine->Speaker, Cue.Face, true);
	if (!Cue.Camera.IsNone()) ExecuteCameraAction(Cue.Camera, Cue.GlyphIndex);
	if (!Cue.Sound.IsNone() && CurrentSequence && CurrentSequence->Theme)
	{
		if (const TSoftObjectPtr<USoundBase>* Sound = CurrentSequence->Theme->Sounds.Find(Cue.Sound))
		{
			if (USoundBase* Loaded = Sound->LoadSynchronous()) UGameplayStatics::PlaySound2D(this, Loaded);
		}
	}
}

void UChopItDialogueSubsystem::BroadcastEvent(const FGameplayTag EventTag, const FName MarkerId, const FName ChoiceId, const FName TargetBinding)
{
	if (!EventTag.IsValid()) return;
	FChopItDialogueEventPayload Payload;
	Payload.Handle = CurrentHandle;
	Payload.Sequence = CurrentSequence;
	Payload.LineId = CurrentLine ? CurrentLine->LineId : NAME_None;
	Payload.SpeakerId = CurrentLine && CurrentLine->Speaker ? CurrentLine->Speaker->SpeakerId : NAME_None;
	Payload.MarkerId = MarkerId;
	Payload.ChoiceId = ChoiceId;
	Payload.TargetBinding = TargetBinding;
	Payload.Target = CurrentContext.ResolveBinding(TargetBinding);
	OnDialogueEvent.Broadcast(EventTag, Payload);
}

void UChopItDialogueSubsystem::ExecuteCameraAction(const FName ActionId, const int32 CueGlyphIndex)
{
	if (ActionId.IsNone() || !CurrentSequence || !CurrentSequence->Theme || !GetLocalPlayer()) return;
	const FChopItDialogueCameraAction* Action = CurrentSequence->Theme->CameraActions.Find(ActionId);
	UChopItCameraDirectorSubsystem* Director = GetLocalPlayer()->GetSubsystem<UChopItCameraDirectorSubsystem>();
	if (!Action || !Director)
	{
		UE_LOG(LogChopIt, Warning, TEXT("Dialogue camera action could not resolve: action=%s line=%s actionFound=%d directorFound=%d."),
			*ActionId.ToString(), CurrentLine ? *CurrentLine->LineId.ToString() : TEXT("None"), Action != nullptr, Director != nullptr);
		return;
	}
	FChopItCameraHandle Handle;
	if (Action->Kind == EChopItDialogueCameraActionKind::Cue)
	{
		UChopItCameraCue* Cue = Action->Cue.LoadSynchronous();
		AChopItCameraAnchor* Anchor = Cast<AChopItCameraAnchor>(CurrentContext.ResolveBinding(Action->AnchorBinding));
		AActor* Subject = CurrentContext.ResolveBinding(Action->SubjectBinding);
		Handle = Director->PushCameraCueWithFieldOfView(
			Cue,
			Anchor,
			Subject,
			Action->FieldOfViewOverride,
			Action->BlendInTimeOverride);
		UE_LOG(LogChopIt, Display,
			TEXT("Dialogue camera cue resolved: action=%s line=%s cue=%s anchor=%s subject=%s handleValid=%d."),
			*ActionId.ToString(), CurrentLine ? *CurrentLine->LineId.ToString() : TEXT("None"),
			*GetNameSafe(Cue), *GetNameSafe(Anchor), *GetNameSafe(Subject), Handle.IsValid());
		if (!Handle.IsValid())
		{
			UE_LOG(LogChopIt, Warning, TEXT("Dialogue camera cue failed to create a handle for action %s."), *ActionId.ToString());
		}
	}
	else if (Action->Kind == EChopItDialogueCameraActionKind::Effect)
	{
		Handle = Director->PushCameraEffect(Action->Effect.LoadSynchronous(), Action->DurationOverride);
	}
	else
	{
		if (Action->bSustainUntilLineEnds)
		{
			StopSustainedCameraShake();
			SustainedCameraShakeAction = ActionId;
			SustainedCameraShakeEndGlyph = MAX_int32;
			bSustainCameraShakeUntilLineEnd = true;
			SustainedCameraShakeElapsed = 0.0f;
			if (State != EChopItDialogueState::Entering) RestartSustainedCameraShake();
			return;
		}
		const int32 ShakeEndGlyph = FindSustainedShakeEndGlyph(CueGlyphIndex);
		if (ShakeEndGlyph != INDEX_NONE)
		{
			StopSustainedCameraShake();
			SustainedCameraShakeAction = ActionId;
			SustainedCameraShakeEndGlyph = ShakeEndGlyph;
			SustainedCameraShakeElapsed = 0.0f;
			// Glyph-zero cues are processed during the panel entrance. Delay the physical
			// shake until the typewriter actually begins revealing the emphasized span.
			if (State == EChopItDialogueState::Revealing) RestartSustainedCameraShake();
			return;
		}
		AActor* OriginActor = CurrentContext.ResolveBinding(Action->SubjectBinding);
		Handle = Director->PlayCameraShake(Action->Shake.LoadSynchronous(), Action->Scale, OriginActor ? OriginActor->GetActorLocation() : FVector::ZeroVector);
	}
	if (Handle.IsValid()) (Action->bPersistUntilDialogueEnds ? PersistentCameraHandles : LineCameraHandles).Add(Handle);
}

int32 UChopItDialogueSubsystem::FindSustainedShakeEndGlyph(const int32 StartGlyph) const
{
	if (!CurrentDocument.Glyphs.IsValidIndex(StartGlyph)
		|| CurrentDocument.Glyphs[StartGlyph].Style.ShakeAmplitude <= KINDA_SMALL_NUMBER)
	{
		return INDEX_NONE;
	}
	int32 EndGlyph = StartGlyph;
	while (CurrentDocument.Glyphs.IsValidIndex(EndGlyph)
		&& CurrentDocument.Glyphs[EndGlyph].Style.ShakeAmplitude > KINDA_SMALL_NUMBER)
	{
		++EndGlyph;
	}
	return EndGlyph;
}

void UChopItDialogueSubsystem::UpdateSustainedCameraShake(const float DeltaSeconds)
{
	if (SustainedCameraShakeEndGlyph == INDEX_NONE) return;
	if (bSustainCameraShakeUntilLineEnd)
	{
		// Glyph-zero cues are discovered while the panel enters. Once the line starts,
		// keep renewing the authored short envelope through reveal, waiting and exit.
		if (State == EChopItDialogueState::Entering) return;
		SustainedCameraShakeElapsed += DeltaSeconds;
		if (!SustainedCameraShakeHandle.IsValid() || SustainedCameraShakeElapsed >= 0.20f)
		{
			RestartSustainedCameraShake();
		}
		return;
	}
	if (State != EChopItDialogueState::Revealing || RevealCount >= SustainedCameraShakeEndGlyph)
	{
		if (State != EChopItDialogueState::Entering) StopSustainedCameraShake();
		return;
	}

	SustainedCameraShakeElapsed += DeltaSeconds;
	// CS_Critical has a short authored envelope. Restart just before it finishes so
	// a slowly typed marked word receives one continuous screen response.
	if (!SustainedCameraShakeHandle.IsValid() || SustainedCameraShakeElapsed >= 0.20f)
	{
		RestartSustainedCameraShake();
	}
}

void UChopItDialogueSubsystem::RestartSustainedCameraShake()
{
	if (!GetLocalPlayer() || !CurrentSequence || !CurrentSequence->Theme || SustainedCameraShakeAction.IsNone()) return;
	UChopItCameraDirectorSubsystem* Director = GetLocalPlayer()->GetSubsystem<UChopItCameraDirectorSubsystem>();
	const FChopItDialogueCameraAction* Action = CurrentSequence->Theme->CameraActions.Find(SustainedCameraShakeAction);
	if (!Director || !Action || Action->Kind != EChopItDialogueCameraActionKind::Shake) return;
	if (SustainedCameraShakeHandle.IsValid()) Director->StopCameraRequest(SustainedCameraShakeHandle);
	AActor* OriginActor = CurrentContext.ResolveBinding(Action->SubjectBinding);
	SustainedCameraShakeHandle = Director->PlayCameraShake(
		Action->Shake.LoadSynchronous(),
		Action->Scale,
		OriginActor ? OriginActor->GetActorLocation() : FVector::ZeroVector);
	SustainedCameraShakeElapsed = 0.0f;
}

void UChopItDialogueSubsystem::StopSustainedCameraShake()
{
	if (SustainedCameraShakeHandle.IsValid() && GetLocalPlayer())
	{
		if (UChopItCameraDirectorSubsystem* Director = GetLocalPlayer()->GetSubsystem<UChopItCameraDirectorSubsystem>())
		{
			Director->StopCameraRequest(SustainedCameraShakeHandle);
		}
	}
	SustainedCameraShakeHandle.Invalidate();
	SustainedCameraShakeAction = NAME_None;
	SustainedCameraShakeEndGlyph = INDEX_NONE;
	SustainedCameraShakeElapsed = 0.0f;
	bSustainCameraShakeUntilLineEnd = false;
}

void UChopItDialogueSubsystem::ClearLineCameraActions()
{
	StopSustainedCameraShake();
	if (GetLocalPlayer()) if (UChopItCameraDirectorSubsystem* Camera = GetLocalPlayer()->GetSubsystem<UChopItCameraDirectorSubsystem>()) for (const FChopItCameraHandle Handle : LineCameraHandles) Camera->StopCameraRequest(Handle);
	LineCameraHandles.Reset();
}

void UChopItDialogueSubsystem::ClearAllCameraActions()
{
	ClearLineCameraActions();
	if (GetLocalPlayer()) if (UChopItCameraDirectorSubsystem* Camera = GetLocalPlayer()->GetSubsystem<UChopItCameraDirectorSubsystem>())
	{
		for (const FChopItCameraHandle Handle : PersistentCameraHandles) Camera->StopCameraRequest(Handle);
		Camera->PopInputLock(InputLockHandle);
	}
	PersistentCameraHandles.Reset();
	InputLockHandle.Invalidate();
}

void UChopItDialogueSubsystem::SetDialogueInputEnabled(const bool bEnabled)
{
	if (!GetLocalPlayer()) return;
	UEnhancedInputLocalPlayerSubsystem* Input = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	const UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/ChopIt/Input/IMC_Dialogue.IMC_Dialogue"));
	if (!Input || !Context) return;
	if (bEnabled) Input->AddMappingContext(Context, 100);
	else Input->RemoveMappingContext(Context);
}

void UChopItDialogueSubsystem::FinishActive(const EChopItDialogueEndReason Reason, const bool bStartNext)
{
	if (!CurrentHandle.IsValid() && !CurrentSequence) return;
	const FChopItDialogueHandle FinishedHandle = CurrentHandle;
	if (CurrentSequence) BroadcastEvent(CurrentSequence->EndEvent);
	if (VoiceComponent) { VoiceComponent->Stop(); VoiceComponent = nullptr; }
	ClearAllCameraActions();
	SetDialogueInputEnabled(false);
	if (bOwnedPause) if (APlayerController* PC = ResolvePlayerController()) PC->SetPause(false);
	bOwnedPause = false;
	if (Widget) { Widget->RemoveFromParent(); Widget = nullptr; }
	CurrentLine = nullptr;
	CurrentSequence = nullptr;
	CurrentContext = {};
	CurrentDocument = {};
	CurrentHandle.Invalidate();
	State = EChopItDialogueState::Closed;
	OnDialogueEnded.Broadcast(FinishedHandle, Reason);
	if (bStartNext && !Queue.IsEmpty())
	{
		const FChopItPendingDialogueRequest Next = Queue[0];
		Queue.RemoveAt(0);
		BeginRequest(Next);
	}
}
