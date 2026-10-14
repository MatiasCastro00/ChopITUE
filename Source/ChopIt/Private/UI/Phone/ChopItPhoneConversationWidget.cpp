#include "UI/Phone/ChopItPhoneConversationWidget.h"
#include "UI/ChopItAnimatedTextBlock.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UChopItPhoneConversationWidget::RebuildWidget()
{
	// The Widget Blueprint owns the tree, including the reusable response cards.
	return UUserWidget::RebuildWidget();
}

void UChopItPhoneConversationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (RefreshButton) RefreshButton->OnClicked.AddUniqueDynamic(this, &UChopItPhoneConversationWidget::ClickRefresh);
	if (SkipButton) SkipButton->OnClicked.AddUniqueDynamic(this, &UChopItPhoneConversationWidget::ClickSkip);
	if (ChoiceButton1) ChoiceButton1->OnClicked.AddUniqueDynamic(this, &UChopItPhoneConversationWidget::ClickChoiceOne);
	if (ChoiceButton2) ChoiceButton2->OnClicked.AddUniqueDynamic(this, &UChopItPhoneConversationWidget::ClickChoiceTwo);
	if (ChoiceButton3) ChoiceButton3->OnClicked.AddUniqueDynamic(this, &UChopItPhoneConversationWidget::ClickChoiceThree);
	if (ChoiceButton1) ChoiceButton1->OnHovered.AddUniqueDynamic(this, &UChopItPhoneConversationWidget::HoverChoiceOne);
	if (ChoiceButton2) ChoiceButton2->OnHovered.AddUniqueDynamic(this, &UChopItPhoneConversationWidget::HoverChoiceTwo);
	if (ChoiceButton3) ChoiceButton3->OnHovered.AddUniqueDynamic(this, &UChopItPhoneConversationWidget::HoverChoiceThree);
}

void UChopItPhoneConversationWidget::NativeDestruct()
{
	if (RefreshButton) RefreshButton->OnClicked.RemoveDynamic(this, &UChopItPhoneConversationWidget::ClickRefresh);
	if (SkipButton) SkipButton->OnClicked.RemoveDynamic(this, &UChopItPhoneConversationWidget::ClickSkip);
	if (ChoiceButton1) ChoiceButton1->OnClicked.RemoveDynamic(this, &UChopItPhoneConversationWidget::ClickChoiceOne);
	if (ChoiceButton2) ChoiceButton2->OnClicked.RemoveDynamic(this, &UChopItPhoneConversationWidget::ClickChoiceTwo);
	if (ChoiceButton3) ChoiceButton3->OnClicked.RemoveDynamic(this, &UChopItPhoneConversationWidget::ClickChoiceThree);
	if (ChoiceButton1) ChoiceButton1->OnHovered.RemoveDynamic(this, &UChopItPhoneConversationWidget::HoverChoiceOne);
	if (ChoiceButton2) ChoiceButton2->OnHovered.RemoveDynamic(this, &UChopItPhoneConversationWidget::HoverChoiceTwo);
	if (ChoiceButton3) ChoiceButton3->OnHovered.RemoveDynamic(this, &UChopItPhoneConversationWidget::HoverChoiceThree);
	Super::NativeDestruct();
}

void UChopItPhoneConversationWidget::ClickRefresh() { if (OnRefreshRequested.IsBound()) OnRefreshRequested.Execute(); }
void UChopItPhoneConversationWidget::ClickSkip() { if (OnSkipRequested.IsBound()) OnSkipRequested.Execute(); }
void UChopItPhoneConversationWidget::ClickChoiceOne() { OnChoiceRequested.ExecuteIfBound(0); }
void UChopItPhoneConversationWidget::ClickChoiceTwo() { OnChoiceRequested.ExecuteIfBound(1); }
void UChopItPhoneConversationWidget::ClickChoiceThree() { OnChoiceRequested.ExecuteIfBound(2); }
void UChopItPhoneConversationWidget::HoverChoiceOne() { OnChoiceHovered.ExecuteIfBound(0); }
void UChopItPhoneConversationWidget::HoverChoiceTwo() { OnChoiceHovered.ExecuteIfBound(1); }
void UChopItPhoneConversationWidget::HoverChoiceThree() { OnChoiceHovered.ExecuteIfBound(2); }

void UChopItPhoneConversationWidget::ConfigureTheme(const UChopItDialogueTheme*, bool ReduceMotion, float UIScale)
{
	bStill = ReduceMotion;
	if (!BodyText) return;
	BodyText->SetDefaultColor(FLinearColor(0.96f, 0.95f, 0.89f));
	BodyText->SetReduceMotion(bStill);
	BodyText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FMath::RoundToInt(26 * FMath::Clamp(UIScale, .75f, 1.75f))));
}

void UChopItPhoneConversationWidget::ShowLine(const FChopItDialogueLine& Line, const FChopItDialogueMarkupDocument& Document)
{
	if (CallerName) CallerName->SetVisibility(ESlateVisibility::Collapsed);
	if (BodyText) BodyText->SetDocument(Document);
	SetChoices({}, 0);
	SetAwaitingAdvance(false);
	ResetTransition();
}

void UChopItPhoneConversationWidget::SetRevealCount(int32 Count)
{
	if (BodyText) BodyText->SetRevealCount(Count);
}

void UChopItPhoneConversationWidget::SetChoices(const TArray<FChopItDialogueChoiceView>& Choices, int32 SelectedIndex)
{
	CurrentChoices = Choices;
	ChoicesElapsed = 0.0f;
	if (ResponseCards) ResponseCards->SetVisibility(Choices.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (RefreshButton) RefreshButton->SetVisibility(Choices.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (SkipButton) SkipButton->SetVisibility(ESlateVisibility::Visible);
	UButton* Buttons[] = {ChoiceButton1, ChoiceButton2, ChoiceButton3};
	for (int32 i = 0; i < 3; ++i) if (Buttons[i]) Buttons[i]->SetVisibility(Choices.IsValidIndex(i) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	// The first choices may arrive before NativeConstruct in an authored WBP.
	if (ChoiceButton1) ChoiceButton1->OnHovered.AddUniqueDynamic(this, &UChopItPhoneConversationWidget::HoverChoiceOne);
	if (ChoiceButton2) ChoiceButton2->OnHovered.AddUniqueDynamic(this, &UChopItPhoneConversationWidget::HoverChoiceTwo);
	if (ChoiceButton3) ChoiceButton3->OnHovered.AddUniqueDynamic(this, &UChopItPhoneConversationWidget::HoverChoiceThree);
	SetSelectedChoice(SelectedIndex);
}

void UChopItPhoneConversationWidget::SetSelectedChoice(int32 SelectedIndex)
{
	HighlightedChoice = SelectedIndex;
	// Page through additional authored choices without allocating visual widgets.
	FirstVisibleChoice = FMath::Max(0, SelectedIndex / 4 * 4);
	UBorder* Cards[] = {Card1, Card2, Card3, Card4};
	UTextBlock* Labels[] = {Response1, Response2, Response3, Response4};
	UTextBlock* Descriptions[] = {CardHint1, CardHint2, CardHint3, CardHint4};
	UTextBlock* Costs[] = {CardCost1, CardCost2, CardCost3, nullptr};
	for (int32 i = 0; i < 4; ++i)
	{
		if (!Cards[i] || !Labels[i]) continue;
		const int32 ChoiceIndex = FirstVisibleChoice + i;
		const bool bExists = CurrentChoices.IsValidIndex(ChoiceIndex);
		Cards[i]->SetVisibility(bExists ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (!bExists) continue;
		FString Benefit, Price;
		const FString Description = CurrentChoices[ChoiceIndex].Description.ToString();
		if (Description.Split(TEXT("\nPRECIO: "), &Benefit, &Price))
		{
			if (Descriptions[i]) Descriptions[i]->SetText(FText::FromString(Benefit));
			if (Costs[i])
			{
				Costs[i]->SetText(FText::FromString(FString(TEXT("PRECIO: ")) + Price));
				Costs[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
		else
		{
			if (Descriptions[i]) Descriptions[i]->SetText(CurrentChoices[ChoiceIndex].Description);
			if (Costs[i]) Costs[i]->SetVisibility(ESlateVisibility::Collapsed);
		}
		Labels[i]->SetText(FText::Format(FText::FromString(TEXT("{0}.  {1}")), FText::AsNumber(ChoiceIndex + 1), CurrentChoices[ChoiceIndex].Text));
		Cards[i]->SetBrushColor(ChoiceIndex == SelectedIndex ? FLinearColor(1.0f, .54f, .04f) : FLinearColor(.075f, .065f, .05f));
	}
}

void UChopItPhoneConversationWidget::SetAwaitingAdvance(bool Awaiting)
{
	if (ContinueHint) ContinueHint->SetVisibility(Awaiting ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UChopItPhoneConversationWidget::SetTransitionProgress(float Alpha, bool Exiting, bool)
{
	if (!SpeechPanel) return;
	const float PanelOpacity = Exiting ? 1 - FMath::Clamp(Alpha, 0.f, 1.f) : FMath::Clamp(Alpha, 0.f, 1.f);
	SpeechPanel->SetRenderOpacity(PanelOpacity);
	SpeechPanel->SetRenderTranslation(FVector2D(bStill ? 0 : 16 * (1 - PanelOpacity), 0));
}

void UChopItPhoneConversationWidget::ResetTransition()
{
	if (SpeechPanel) { SpeechPanel->SetRenderOpacity(1); SpeechPanel->SetRenderTranslation(FVector2D::ZeroVector); }
}

void UChopItPhoneConversationWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	UUserWidget::NativeTick(Geometry, DeltaTime);
	if (BodyText)
	{
		const float Width = BodyText->GetCachedGeometry().GetLocalSize().X;
		if (Width > 50) BodyText->SetWrapAt(Width);
	}
	ChoicesElapsed += DeltaTime;
	UBorder* Cards[] = {Card1, Card2, Card3, Card4};
	for (int32 i = 0; i < 4; ++i)
	{
		if (!Cards[i]) continue;
		const float T = bStill ? 1.f : FMath::Clamp((ChoicesElapsed - i * .065f) / .20f, 0.f, 1.f);
		Cards[i]->SetRenderOpacity(T);
		Cards[i]->SetRenderTranslation(FVector2D(0, 18 * FMath::Square(1 - T)));
		const float TargetScale = HighlightedChoice == FirstVisibleChoice + i ? 1.025f : 1.f;
		const FVector2D CurrentScale = Cards[i]->GetRenderTransform().Scale;
		Cards[i]->SetRenderScale(FMath::Vector2DInterpTo(CurrentScale, FVector2D(TargetScale), DeltaTime, 14.f));
	}
}
