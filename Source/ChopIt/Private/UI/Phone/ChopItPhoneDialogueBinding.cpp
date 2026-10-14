#include "UI/Phone/ChopItPhoneDialogueBinding.h"

#include "UI/Phone/ChopItPhoneDialogueScene.h"
#include "UI/ChopItHUD.h"
#include "UI/Phone/ChopItPhonePresentationViewModel.h"
#include "UI/Phone/ChopItPhonePresentationWidget.h"
#include "UI/Phone/ChopItPhoneConversationWidget.h"
#include "Framework/ChopItPlayerState.h"
#include "Combat/ChopItCombatStatsComponent.h"
#include "Combat/ChopItHealthComponent.h"
#include "Phone/ChopItPhoneBargainDefinition.h"
#include "Progression/ChopItUpgradeOfferComponent.h"
#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneVariantComponent.h"
#include "Phone/ChopItPhoneVariantData.h"
#include "Dialogue/ChopItDialogueAssets.h"
#include "Dialogue/ChopItDialogueSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

bool UChopItPhoneDialogueBinding::IsPresentationActive() const
{
	return IsValid(Scene.Get()) && IsValid(Widget.Get());
}

void UChopItPhoneDialogueBinding::Initialize(APlayerController* InPlayer, AChopItMysteryPhone* InPhone)
{
	Player = InPlayer;
	Phone = InPhone;
	if (!InPlayer || !InPhone || !InPlayer->IsLocalController() || !InPlayer->GetLocalPlayer() || !InPhone->GetPhoneState()) return;
	Dialogue = InPlayer->GetLocalPlayer()->GetSubsystem<UChopItDialogueSubsystem>();
	if (!Dialogue) return;
	InPhone->GetPhoneState()->OnStateChanged.AddDynamic(this, &UChopItPhoneDialogueBinding::HandlePhoneStateChanged);
	Dialogue->OnDialogueEnded.AddDynamic(this, &UChopItPhoneDialogueBinding::HandleDialogueEnded);
	Dialogue->OnChoiceSelected.AddDynamic(this, &UChopItPhoneDialogueBinding::HandleChoiceSelected);
	if (InPhone->GetPhoneState()->GetState() == EChopItPhoneState::Dialogue) StartTicker();
}

void UChopItPhoneDialogueBinding::Shutdown()
{
	bShuttingDown = true;
	if (Phone.IsValid() && Phone->GetPhoneState())
		Phone->GetPhoneState()->OnStateChanged.RemoveDynamic(this, &UChopItPhoneDialogueBinding::HandlePhoneStateChanged);
	if (Dialogue)
	{
		Dialogue->OnDialogueEnded.RemoveDynamic(this, &UChopItPhoneDialogueBinding::HandleDialogueEnded);
		Dialogue->OnChoiceSelected.RemoveDynamic(this, &UChopItPhoneDialogueBinding::HandleChoiceSelected);
		if (ActiveHandle.IsValid()) Dialogue->StopDialogue(ActiveHandle);
	}
	ActiveHandle.Invalidate();
	ClearPresentation();
	Dialogue = nullptr;
	Phone.Reset();
	Player.Reset();
}

void UChopItPhoneDialogueBinding::HandlePhoneStateChanged(EChopItPhoneState, EChopItPhoneState Current)
{
	if (Current == EChopItPhoneState::Dialogue) StartTicker();
	else if (Current == EChopItPhoneState::Completed) ClearPresentation();
}

void UChopItPhoneDialogueBinding::StartTicker()
{
	if (!TickerHandle.IsValid())
		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UChopItPhoneDialogueBinding::Tick));
	TryStartDialogue();
}

bool UChopItPhoneDialogueBinding::Tick(float DeltaSeconds)
{
	if (bShuttingDown || !Phone.IsValid() || !Player.IsValid() || !Dialogue ||
		!Phone->GetPhoneState() || Phone->GetPhoneState()->GetState() != EChopItPhoneState::Dialogue)
	{
		ClearPresentation();
		return false;
	}
	if (!ActiveHandle.IsValid()) TryStartDialogue();
	if (Scene)
	{
		PresentationElapsed += DeltaSeconds;
		Scene->UpdatePresentation(PresentationElapsed);
	}
	return true;
}

void UChopItPhoneDialogueBinding::TryStartDialogue()
{
	if (bShuttingDown || ActiveHandle.IsValid() || !Phone.IsValid() || !Player.IsValid() || !Dialogue ||
		Dialogue->IsDialogueActive() || !Phone->GetPhoneVariant() || !Phone->GetPhoneState() ||
		Phone->GetPhoneState()->GetState() != EChopItPhoneState::Dialogue) return;
	const UChopItPhoneVariantData* Variant = Phone->GetPhoneVariant()->GetSelectedVariant();
	UChopItDialogueSequence* Sequence = Variant ? Variant->DialogueSequence.LoadSynchronous() : nullptr;
	TArray<FText> Errors;
	if (!Sequence || !Sequence->ValidateSequence(Errors))
	{
		UE_LOG(LogTemp, Warning, TEXT("Phone %s has no valid dialogue sequence."), *GetNameSafe(Phone.Get()));
		Phone->GetPhoneState()->TryTransition(EChopItPhoneState::Completed);
		return;
	}
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Scene = Player->GetWorld()->SpawnActor<AChopItPhoneDialogueScene>(
		AChopItPhoneDialogueScene::StaticClass(), FVector(0, 0, 100000), FRotator::ZeroRotator, Params);
	if (!Scene || !Scene->InitializeScene(Phone.Get(), Variant))
	{
		UE_LOG(LogTemp, Warning, TEXT("Phone 3D presentation could not initialize."));
		Phone->GetPhoneState()->TryTransition(EChopItPhoneState::Completed);
		ClearPresentation();
		return;
	}
	ViewModel = NewObject<UChopItPhonePresentationViewModel>(this);
	ViewModel->SetVariantName(Variant->DisplayName);
	ViewModel->SetAccentColor(Variant->AccentColor);
	ViewModel->SetPortraitMaterial(Scene->GetCompositeMaterial());
	UClass* PresentationClass = LoadClass<UChopItPhonePresentationWidget>(nullptr,
		TEXT("/Game/ChopIt/Phone/WBP_PhonePresentation.WBP_PhonePresentation_C"));
	Widget = PresentationClass ? CreateWidget<UChopItPhonePresentationWidget>(Player.Get(), PresentationClass) : nullptr;
	if (!Widget)
	{
		Phone->GetPhoneState()->TryTransition(EChopItPhoneState::Completed);
		ClearPresentation();
		return;
	}
	Widget->SetViewModel(ViewModel);
	Widget->AddToPlayerScreen(900);
	if (AChopItHUD* HUD = Player->GetHUD<AChopItHUD>()) HUD->SetPhoneDialogueActive(true);
	FChopItDialogueContext Context;
	Context.Bindings.Add({TEXT("Phone"), Phone.Get()});
	Context.Bindings.Add({TEXT("Player"), Player->GetPawn()});
	ActiveHandle = Dialogue->StartDialogue(Sequence, Context, EChopItDialogueStartPolicy::RejectIfBusy);
	if (!ActiveHandle.IsValid() || Dialogue->GetActiveSequence() != Sequence)
	{
		ActiveHandle.Invalidate();
		Phone->GetPhoneState()->TryTransition(EChopItPhoneState::Completed);
		ClearPresentation();
		return;
	}
	OfferRandom.Initialize(FMath::Rand());
	ConversationWidget = Cast<UChopItPhoneConversationWidget>(Dialogue->GetWidget());
	if (!ConversationWidget || !BuildOfferChoices())
	{
		Phone->GetPhoneState()->TryTransition(EChopItPhoneState::Completed);
		ClearPresentation();
		return;
	}
	ConversationWidget->OnRefreshRequested.BindUObject(this, &UChopItPhoneDialogueBinding::RerollOffers);
	ConversationWidget->OnSkipRequested.BindUObject(this, &UChopItPhoneDialogueBinding::SkipOffer);
	ConversationWidget->OnChoiceRequested.BindUObject(this, &UChopItPhoneDialogueBinding::ChooseOffer);
	ConversationWidget->OnChoiceHovered.BindUObject(this, &UChopItPhoneDialogueBinding::HighlightOffer);
	bPreviousMouseCursor = Player->bShowMouseCursor;
	Player->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	Player->SetInputMode(InputMode);
}

bool UChopItPhoneDialogueBinding::BuildOfferChoices()
{
	if (!Player.IsValid() || !Dialogue || !ActiveHandle.IsValid()) return false;
	static const TCHAR* Names[] = {TEXT("HachaDeVidrio"), TEXT("SierraFrenetica"), TEXT("BrazoInfinito"),
		TEXT("FiloHambriento"), TEXT("TroncosMalditos"), TEXT("PiesPrestados"), TEXT("VerdugoCercano"),
		TEXT("FortunaDeSangre"), TEXT("UltimoAliento"), TEXT("GuardiaInmovil")};
	TArray<UChopItPhoneBargainDefinition*> Candidates;
	for (const TCHAR* Name : Names)
	{
		const FString Path = FString::Printf(TEXT("/Game/ChopIt/Phone/Bargains/DA_PhoneBargain_%s.DA_PhoneBargain_%s"), Name, Name);
		if (UChopItPhoneBargainDefinition* Bargain = LoadObject<UChopItPhoneBargainDefinition>(nullptr, *Path))
			if (!Bargain->BargainId.IsNone() && !Bargain->Modifiers.IsEmpty() &&
				!Bargain->BenefitText.IsEmpty() && !Bargain->CostText.IsEmpty()) Candidates.Add(Bargain);
	}
	if (Candidates.Num() < 3) return false;
	TArray<TObjectPtr<UChopItPhoneBargainDefinition>> NewOffers;
	TArray<FChopItDialogueChoice> Choices;
	while (Choices.Num() < 3)
	{
		float TotalWeight = 0;
		for (const UChopItPhoneBargainDefinition* Candidate : Candidates) TotalWeight += FMath::Max(.001f, Candidate->OfferWeight);
		float Roll = OfferRandom.FRandRange(0, TotalWeight);
		int32 Pick = Candidates.Num() - 1;
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			Roll -= FMath::Max(.001f, Candidates[Index]->OfferWeight);
			if (Roll <= 0) { Pick = Index; break; }
		}
		UChopItPhoneBargainDefinition* Bargain = Candidates[Pick];
		NewOffers.Add(Bargain);
		FChopItDialogueChoice Choice;
		Choice.ChoiceId = Bargain->BargainId;
		Choice.Text = Bargain->DisplayName;
		Choice.Description = FText::FromString(FString::Printf(TEXT("BENEFICIO: %s\nPRECIO: %s"),
			*Bargain->BenefitText.ToString(), *Bargain->CostText.ToString()));
		Choices.Add(Choice);
		Candidates.RemoveAtSwap(Pick);
	}
	if (!Dialogue->SetChoiceOverrides(ActiveHandle, TEXT("Offer"), Choices)) return false;
	CurrentOffers = MoveTemp(NewOffers);
	return true;
}

bool UChopItPhoneDialogueBinding::RerollOffers()
{
	if (!Dialogue || Dialogue->GetState() != EChopItDialogueState::ShowingChoices) return false;
	TArray<FName> Previous;
	for (const UChopItPhoneBargainDefinition* Bargain : CurrentOffers) if (Bargain) Previous.Add(Bargain->BargainId);
	for (int32 Attempt = 0; Attempt < 6; ++Attempt)
	{
		if (!BuildOfferChoices()) return false;
		bool bChanged = false;
		for (const UChopItPhoneBargainDefinition* Bargain : CurrentOffers)
			bChanged |= Bargain && !Previous.Contains(Bargain->BargainId);
		if (bChanged) return true;
	}
	return true;
}

bool UChopItPhoneDialogueBinding::SkipOffer()
{
	return Dialogue && ActiveHandle.IsValid() && Dialogue->CancelDialogue();
}

void UChopItPhoneDialogueBinding::ChooseOffer(const int32 Index)
{
	if (Dialogue && ActiveHandle.IsValid()) Dialogue->SelectChoice(Index);
}

void UChopItPhoneDialogueBinding::HighlightOffer(const int32 Index)
{
	if (Dialogue && ActiveHandle.IsValid()) Dialogue->SetChoiceSelection(Index);
}

void UChopItPhoneDialogueBinding::HandleChoiceSelected(FChopItDialogueHandle Handle, FName LineId, FName ChoiceId)
{
	if (!(Handle == ActiveHandle) || LineId != TEXT("Offer") || !Player.IsValid()) return;
	AChopItPlayerState* State = Player->GetPlayerState<AChopItPlayerState>();
	UChopItUpgradeOfferComponent* Progression = State ? State->GetUpgradeOfferComponent() : nullptr;
	UChopItCombatStatsComponent* Stats = Player->GetPawn() ? Player->GetPawn()->FindComponentByClass<UChopItCombatStatsComponent>() : nullptr;
	UChopItHealthComponent* Health = Player->GetPawn() ? Player->GetPawn()->FindComponentByClass<UChopItHealthComponent>() : nullptr;
	for (UChopItPhoneBargainDefinition* Bargain : CurrentOffers)
	{
		if (Bargain && Bargain->BargainId == ChoiceId && Progression && Stats)
		{
			if (!Progression->ApplyPhoneBargain(Bargain, Stats, Health)) UE_LOG(LogTemp, Warning, TEXT("Phone bargain %s could not be applied"), *ChoiceId.ToString());
			return;
		}
	}
}

void UChopItPhoneDialogueBinding::HandleDialogueEnded(FChopItDialogueHandle Handle, EChopItDialogueEndReason)
{
	if (bShuttingDown || !(Handle == ActiveHandle)) return;
	ActiveHandle.Invalidate();
	if (Phone.IsValid() && Phone->GetPhoneState())
		Phone->GetPhoneState()->TryTransition(EChopItPhoneState::Completed);
	ClearPresentation();
}

void UChopItPhoneDialogueBinding::ClearPresentation()
{
	if (ConversationWidget)
	{
		ConversationWidget->OnRefreshRequested.Unbind();
		ConversationWidget->OnSkipRequested.Unbind();
		ConversationWidget->OnChoiceRequested.Unbind();
		ConversationWidget->OnChoiceHovered.Unbind();
		ConversationWidget = nullptr;
	}
	CurrentOffers.Reset();
	if (Player.IsValid() && Widget)
	{
		Player->bShowMouseCursor = bPreviousMouseCursor;
		FInputModeGameOnly InputMode;
		Player->SetInputMode(InputMode);
	}
	if (Dialogue && ActiveHandle.IsValid())
	{
		const FChopItDialogueHandle HandleToStop = ActiveHandle;
		ActiveHandle.Invalidate();
		Dialogue->StopDialogue(HandleToStop);
	}
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	if (Widget)
	{
		if (Player.IsValid())
			if (AChopItHUD* HUD = Player->GetHUD<AChopItHUD>()) HUD->SetPhoneDialogueActive(false);
		Widget->SetViewModel(nullptr);
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
	ViewModel = nullptr;
	if (IsValid(Scene)) Scene->Destroy();
	Scene = nullptr;
	PresentationElapsed = 0.0f;
}
