#pragma once

#include "UObject/Object.h"
#include "Containers/Ticker.h"
#include "Dialogue/ChopItDialogueTypes.h"
#include "Phone/ChopItPhoneStateComponent.h"
#include "ChopItPhoneDialogueBinding.generated.h"

class AChopItMysteryPhone;
class AChopItPhoneDialogueScene;
class APlayerController;
class UChopItDialogueSubsystem;
class UChopItPhonePresentationViewModel;
class UChopItPhonePresentationWidget;
class UChopItPhoneConversationWidget;
class UChopItPhoneBargainDefinition;

/** Adapts authoritative phone state to the existing dialogue and 3D UI systems. */
UCLASS()
class CHOPIT_API UChopItPhoneDialogueBinding final : public UObject
{
	GENERATED_BODY()
public:
	void Initialize(APlayerController* InPlayer, AChopItMysteryPhone* InPhone);
	void Shutdown();
	AChopItMysteryPhone* GetPhone() const { return Phone.Get(); }
	bool IsPresentationActive() const;
	bool RerollOffers();
	bool SkipOffer();
private:
	UFUNCTION() void HandlePhoneStateChanged(EChopItPhoneState Previous, EChopItPhoneState Current);
	UFUNCTION() void HandleDialogueEnded(FChopItDialogueHandle Handle, EChopItDialogueEndReason Reason);
	UFUNCTION() void HandleChoiceSelected(FChopItDialogueHandle Handle, FName LineId, FName ChoiceId);
	void ChooseOffer(int32 Index);
	void HighlightOffer(int32 Index);
	bool BuildOfferChoices();
	bool Tick(float DeltaSeconds);
	void StartTicker();
	void TryStartDialogue();
	void ClearPresentation();
	TWeakObjectPtr<APlayerController> Player;
	TWeakObjectPtr<AChopItMysteryPhone> Phone;
	UPROPERTY(Transient) TObjectPtr<UChopItDialogueSubsystem> Dialogue;
	UPROPERTY(Transient) TObjectPtr<AChopItPhoneDialogueScene> Scene;
	UPROPERTY(Transient) TObjectPtr<UChopItPhonePresentationViewModel> ViewModel;
	UPROPERTY(Transient) TObjectPtr<UChopItPhonePresentationWidget> Widget;
	UPROPERTY(Transient) TObjectPtr<UChopItPhoneConversationWidget> ConversationWidget;
	UPROPERTY(Transient) TArray<TObjectPtr<UChopItPhoneBargainDefinition>> CurrentOffers;
	FChopItDialogueHandle ActiveHandle;
	FRandomStream OfferRandom;
	bool bPreviousMouseCursor = false;
	FTSTicker::FDelegateHandle TickerHandle;
	float PresentationElapsed = 0.0f;
	bool bShuttingDown = false;
};
