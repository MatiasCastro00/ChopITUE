#pragma once

#include "Dialogue/ChopItDialogueTypes.h"
#include "GameFramework/PlayerController.h"
#include "ChopItPlayerController.generated.h"

/** Owns local input context and interaction intent, not gameplay rules. */
UCLASS()
class CHOPIT_API AChopItPlayerController final : public APlayerController
{
	GENERATED_BODY()

public:
	AChopItPlayerController();
	virtual void GetAudioListenerPosition(FVector& OutLocation, FVector& OutFrontDir, FVector& OutRightDir) const override;
	virtual bool GetAudioListenerAttenuationOverridePosition(FVector& OutLocation) const override;

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

private:
	void TryStartMatchIntro();
	void EnsureStartupDeliveryActors(class AChopItQuotaMachine* Oven);
	class AChopItCameraAnchor* SpawnIntroCameraAnchor(
		FName Name,
		AActor* Subject,
		const FVector& Offset,
		const FVector& SubjectFocusOffset = FVector::ZeroVector) const;
	UFUNCTION() void HandleIntroDialogueEvent(FGameplayTag EventTag, FChopItDialogueEventPayload Payload);
	UFUNCTION() void HandleIntroDialogueEnded(FChopItDialogueHandle Handle, EChopItDialogueEndReason Reason);
	void DialogueAdvance();
	void DialogueNextChoice();
	void DialoguePreviousChoice();
	void DialogueCancel();
	void SelectUpgradeOne();
	void SelectUpgradeTwo();
	void SelectUpgradeThree();
	void RerollPhoneOffer();
	void SelectUpgrade(int32 Index);
	void SelectChoiceWithMouse();
	void MoveChoiceLeft();
	void MoveChoiceRight();
	void ConfirmChoice();
	void HandleChoiceStick(float Value);
	void CloseShop();
	void SpawnRewardChestAhead();
	void ToggleDebugItemMenu();
	void DebugItemUp();
	void DebugItemDown();
	void DebugItemPageUp();
	void DebugItemPageDown();
	void DebugItemGrant();

	UPROPERTY(Transient) TObjectPtr<class UInputAction> DialogueAdvanceAction;
	UPROPERTY(Transient) TObjectPtr<class UInputAction> DialogueNextChoiceAction;
	UPROPERTY(Transient) TObjectPtr<class UInputAction> DialoguePreviousChoiceAction;
	UPROPERTY(Transient) TObjectPtr<class UInputAction> DialogueCancelAction;
	UPROPERTY(Transient) TObjectPtr<class AChopItDialogueStageCharacter> IntroDeath;
	UPROPERTY(Transient) TObjectPtr<class AChopItDialogueStageCharacter> IntroOvenPortrait;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> IntroCameraAnchors;
	FTimerHandle MatchIntroTimer;
	bool bMatchIntroAttempted = false;
	bool bChoiceCursorOwned = false;
	bool bPreviousChoiceCursor = false;
	bool bChoiceStickHeld = false;
};
