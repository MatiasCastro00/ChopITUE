#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Dialogue/ChopItDialogueAssets.h"
#include "Dialogue/ChopItDialogueMarkup.h"
#include "Dialogue/ChopItDialogueTypes.h"
#include "ChopItDialogueSubsystem.generated.h"

class UAudioComponent;
class UChopItDialogueWidget;

USTRUCT()
struct FChopItPendingDialogueRequest
{
	GENERATED_BODY()

	UPROPERTY() FChopItDialogueHandle Handle;
	UPROPERTY() TObjectPtr<UChopItDialogueSequence> Sequence;
	UPROPERTY() FChopItDialogueContext Context;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChopItDialogueStartedSignature, FChopItDialogueHandle, Handle, UChopItDialogueSequence*, Sequence);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChopItDialogueEndedSignature, FChopItDialogueHandle, Handle, EChopItDialogueEndReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChopItDialogueLineSignature, FChopItDialogueHandle, Handle, FName, LineId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FChopItDialogueChoiceSignature, FChopItDialogueHandle, Handle, FName, LineId, FName, ChoiceId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChopItDialogueEventSignature, FGameplayTag, EventTag, FChopItDialogueEventPayload, Payload);

/** Local, real-time dialogue state machine. It remains responsive while gameplay is paused. */
UCLASS()
class CHOPITPRESENTATION_API UChopItDialogueSubsystem final : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="ChopIt|Dialogue")
	FChopItDialogueHandle StartDialogue(UChopItDialogueSequence* Sequence, const FChopItDialogueContext& Context, EChopItDialogueStartPolicy Policy = EChopItDialogueStartPolicy::Queue);

	UFUNCTION(BlueprintCallable, Category="ChopIt|Dialogue") void AdvanceOrComplete();
	UFUNCTION(BlueprintCallable, Category="ChopIt|Dialogue") void MoveChoice(int32 Direction);
	UFUNCTION(BlueprintCallable, Category="ChopIt|Dialogue") bool SelectChoice(int32 VisibleChoiceIndex = -1);
	UFUNCTION(BlueprintCallable, Category="ChopIt|Dialogue") bool CancelDialogue();
	UFUNCTION(BlueprintCallable, Category="ChopIt|Dialogue") bool StopDialogue(FChopItDialogueHandle Handle);
	UFUNCTION(BlueprintPure, Category="ChopIt|Dialogue") bool IsDialogueActive() const { return State != EChopItDialogueState::Closed; }
	UFUNCTION(BlueprintPure, Category="ChopIt|Dialogue") EChopItDialogueState GetState() const { return State; }
	UFUNCTION(BlueprintPure, Category="ChopIt|Dialogue") UChopItDialogueSequence* GetActiveSequence() const { return CurrentSequence; }

	UPROPERTY(BlueprintAssignable, Category="ChopIt|Dialogue") FChopItDialogueStartedSignature OnDialogueStarted;
	UPROPERTY(BlueprintAssignable, Category="ChopIt|Dialogue") FChopItDialogueEndedSignature OnDialogueEnded;
	UPROPERTY(BlueprintAssignable, Category="ChopIt|Dialogue") FChopItDialogueLineSignature OnLineStarted;
	UPROPERTY(BlueprintAssignable, Category="ChopIt|Dialogue") FChopItDialogueLineSignature OnLineFinished;
	UPROPERTY(BlueprintAssignable, Category="ChopIt|Dialogue") FChopItDialogueChoiceSignature OnChoiceSelected;
	UPROPERTY(BlueprintAssignable, Category="ChopIt|Dialogue") FChopItDialogueEventSignature OnDialogueEvent;

private:
	bool Tick(float DeltaSeconds);
	void BeginRequest(const FChopItPendingDialogueRequest& Request);
	void EnterLine(FName LineId, bool bFirstLine);
	void FinishReveal(bool bFastForward);
	void BeginAdvance(FName NextLineId, EChopItDialogueEndReason EndReason = EChopItDialogueEndReason::Completed);
	void FinishActive(EChopItDialogueEndReason Reason, bool bStartNext = true);
	void ProcessCues(int32 UpToGlyph, bool bFastForward);
	void ExecuteCue(const FChopItDialogueMarkupCue& Cue);
	void ExecuteCameraAction(FName ActionId, int32 CueGlyphIndex = INDEX_NONE);
	int32 FindSustainedShakeEndGlyph(int32 StartGlyph) const;
	void UpdateSustainedCameraShake(float DeltaSeconds);
	void RestartSustainedCameraShake();
	void StopSustainedCameraShake();
	void BroadcastEvent(FGameplayTag EventTag, FName MarkerId = NAME_None, FName ChoiceId = NAME_None, FName TargetBinding = NAME_None);
	void RefreshChoices();
	void ClearLineCameraActions();
	void ClearAllCameraActions();
	void SetDialogueInputEnabled(bool bEnabled);
	class APlayerController* ResolvePlayerController() const;
	void HandleWorldCleanup(class UWorld* World, bool bSessionEnded, bool bCleanupResources);

	UPROPERTY(Transient) TObjectPtr<UChopItDialogueSequence> CurrentSequence;
	UPROPERTY(Transient) TObjectPtr<UChopItDialogueWidget> Widget;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> VoiceComponent;
	UPROPERTY(Transient) TArray<FChopItPendingDialogueRequest> Queue;
	UPROPERTY(Transient) TArray<FChopItCameraHandle> LineCameraHandles;
	UPROPERTY(Transient) TArray<FChopItCameraHandle> PersistentCameraHandles;

	FChopItDialogueHandle CurrentHandle;
	FChopItDialogueContext CurrentContext;
	FChopItDialogueMarkupDocument CurrentDocument;
	const FChopItDialogueLine* CurrentLine = nullptr;
	TArray<int32> AvailableChoiceIndices;
	FChopItCameraHandle InputLockHandle;
	FChopItCameraHandle SustainedCameraShakeHandle;
	FTSTicker::FDelegateHandle TickerHandle;
	FDelegateHandle WorldCleanupHandle;
	FName PendingLineId;
	FName SustainedCameraShakeAction;
	EChopItDialogueState State = EChopItDialogueState::Closed;
	EChopItDialogueEndReason PendingEndReason = EChopItDialogueEndReason::Completed;
	int32 RevealCount = 0;
	int32 CueCursor = 0;
	int32 SelectedChoice = 0;
	int32 VisitCount = 0;
	int32 BlipCounter = 0;
	float TimeUntilNextGlyph = 0.0f;
	float StateElapsed = 0.0f;
	float AutoAdvanceElapsed = 0.0f;
	float SustainedCameraShakeElapsed = 0.0f;
	int32 SustainedCameraShakeEndGlyph = INDEX_NONE;
	bool bSustainCameraShakeUntilLineEnd = false;
	bool bOwnedPause = false;
	bool bFirstLineTransition = false;
	bool bEndingAfterExit = false;
};
