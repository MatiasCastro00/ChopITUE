#include "UI/ChopItDialogueWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SafeZone.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "UI/ChopItAnimatedTextBlock.h"

TSharedRef<SWidget> UChopItDialogueWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) BuildWidgetTree();
	return Super::RebuildWidget();
}

void UChopItDialogueWidget::BuildWidgetTree()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DialogueRoot"));
	WidgetTree->RootWidget = Root;
	SafeZone = WidgetTree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("DialogueSafeZone"));
	SafeSlot = Root->AddChildToCanvas(SafeZone);
	SafeSlot->SetAnchors(FAnchors(0.045f, 0.60f, 0.955f, 0.965f));
	SafeSlot->SetOffsets(FMargin(0.0f));

	OuterBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OuterBorder"));
	OuterBorder->SetPadding(FMargin(5.0f));
	OuterBorder->SetBrushColor(FLinearColor(1.0f, 0.32f, 0.035f, 1.0f));
	SafeZone->SetContent(OuterBorder);

	InnerBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InnerBorder"));
	InnerBorder->SetPadding(FMargin(20.0f, 16.0f));
	InnerBorder->SetBrushColor(FLinearColor(0.055f, 0.035f, 0.02f, 0.97f));
	OuterBorder->SetContent(InnerBorder);

	DialogueRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DialogueRow"));
	InnerBorder->SetContent(DialogueRow);

	PortraitBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PortraitBox"));
	PortraitBox->SetWidthOverride(220.0f);
	PortraitBox->SetHeightOverride(250.0f);
	PortraitBox->SetClipping(EWidgetClipping::ClipToBoundsAlways);
	PortraitFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PortraitFrame"));
	PortraitFrame->SetPadding(FMargin(3.0f));
	PortraitFrame->SetBrushColor(FLinearColor(1.0f, 0.32f, 0.035f, 1.0f));
	PortraitBox->SetContent(PortraitFrame);
	PortraitBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PortraitBackdrop"));
	PortraitBackdrop->SetPadding(FMargin(7.0f, 6.0f, 7.0f, 0.0f));
	PortraitBackdrop->SetBrushColor(FLinearColor(0.018f, 0.010f, 0.006f, 0.98f));
	PortraitFrame->SetContent(PortraitBackdrop);
	UScaleBox* PortraitScaler = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("PortraitScaler"));
	PortraitScaler->SetStretch(EStretch::ScaleToFit);
	PortraitScaler->SetStretchDirection(EStretchDirection::DownOnly);
	PortraitBackdrop->SetContent(PortraitScaler);
	PortraitOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PortraitOverlay"));
	PortraitScaler->SetContent(PortraitOverlay);
	if (UScaleBoxSlot* ScalerSlot = Cast<UScaleBoxSlot>(PortraitOverlay->Slot))
	{
		ScalerSlot->SetHorizontalAlignment(HAlign_Center);
		ScalerSlot->SetVerticalAlignment(VAlign_Bottom);
	}
	PreviousPortrait = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PreviousPortrait"));
	CurrentPortrait = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CurrentPortrait"));
	UOverlaySlot* PreviousSlot = PortraitOverlay->AddChildToOverlay(PreviousPortrait);
	PreviousSlot->SetHorizontalAlignment(HAlign_Center);
	PreviousSlot->SetVerticalAlignment(VAlign_Bottom);
	UOverlaySlot* CurrentSlot = PortraitOverlay->AddChildToOverlay(CurrentPortrait);
	CurrentSlot->SetHorizontalAlignment(HAlign_Center);
	CurrentSlot->SetVerticalAlignment(VAlign_Bottom);

	TextArea = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TextArea"));
	TextArea->SetClipping(EWidgetClipping::ClipToBoundsAlways);
	SpeakerName = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpeakerName"));
	SpeakerName->SetText(FText::GetEmpty());
	SpeakerName->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.42f, 0.05f)));
	TextArea->AddChildToVerticalBox(SpeakerName)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	USizeBox* TextSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TextSize"));
	TextSize->SetMinDesiredHeight(72.0f);
	AnimatedText = WidgetTree->ConstructWidget<UChopItAnimatedTextBlock>(UChopItAnimatedTextBlock::StaticClass(), TEXT("AnimatedText"));
	TextSize->SetContent(AnimatedText);
	UVerticalBoxSlot* TextSlot = TextArea->AddChildToVerticalBox(TextSize);
	// Keep the precomputed text layout at its desired height. A Fill slot consumed
	// every spare pixel in expanded choice panels and pushed the answers to the
	// bottom, far away from the final paragraph.
	TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	ChoiceBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChoiceBox"));
	TextArea->AddChildToVerticalBox(ChoiceBox)->SetPadding(FMargin(0.0f, 5.0f));
	AdvanceIndicator = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AdvanceIndicator"));
	AdvanceIndicator->SetText(FText::FromString(TEXT("▼")));
	AdvanceIndicator->SetJustification(ETextJustify::Right);
	AdvanceIndicator->SetVisibility(ESlateVisibility::Collapsed);
	TextArea->AddChildToVerticalBox(AdvanceIndicator);

	ArrangePortrait(EChopItDialoguePortraitSide::Left);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UChopItDialogueWidget::ArrangePortrait(const EChopItDialoguePortraitSide Side)
{
	if (!DialogueRow || !PortraitBox || !TextArea) return;
	DialogueRow->ClearChildren();
	auto AddPortrait = [this](const bool bOnLeft)
	{
		UHorizontalBoxSlot* Slot = DialogueRow->AddChildToHorizontalBox(PortraitBox);
		Slot->SetPadding(bOnLeft
			? FMargin(2.0f, 4.0f, 18.0f, 4.0f)
			: FMargin(18.0f, 4.0f, 2.0f, 4.0f));
		// Portraits belong to the speaker header/text group. Bottom alignment made
		// them sink when the panel expanded upward to make room for choices.
		Slot->SetVerticalAlignment(VAlign_Top);
	};
	auto AddText = [this]()
	{
		UHorizontalBoxSlot* Slot = DialogueRow->AddChildToHorizontalBox(TextArea);
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Slot->SetVerticalAlignment(VAlign_Fill);
	};
	if (Side == EChopItDialoguePortraitSide::Left) { AddPortrait(true); AddText(); }
	else { AddText(); AddPortrait(false); }
}

void UChopItDialogueWidget::ConfigureTheme(const UChopItDialogueTheme* InTheme, const bool bInReduceMotion, const float InUIScale)
{
	Theme = InTheme;
	bReduceMotion = bInReduceMotion;
	if (!OuterBorder) return;
	const float UIScale = FMath::Clamp(InUIScale, 0.75f, 1.75f);
	CachedUIScale = UIScale;
	const FLinearColor Border = InTheme ? InTheme->BorderColor : FLinearColor(1.0f, 0.32f, 0.035f, 1.0f);
	const FLinearColor Panel = InTheme ? InTheme->PanelColor : FLinearColor(0.055f, 0.035f, 0.02f, 0.97f);
	const FLinearColor Text = InTheme ? InTheme->TextColor : FLinearColor(0.96f, 0.88f, 0.69f, 1.0f);
	CachedChoiceColor = InTheme ? InTheme->ChoiceColor : CachedChoiceColor;
	CachedSelectedColor = InTheme ? InTheme->ChoiceSelectedColor : CachedSelectedColor;
	OuterBorder->SetBrushColor(Border);
	InnerBorder->SetBrushColor(Panel);
	if (PortraitFrame) PortraitFrame->SetBrushColor(Border);
	if (PortraitBackdrop)
	{
		const FLinearColor PortraitPanel = FLinearColor(
			Panel.R * 0.38f,
			Panel.G * 0.38f,
			Panel.B * 0.38f,
			FMath::Max(0.94f, Panel.A));
		PortraitBackdrop->SetBrushColor(PortraitPanel);
	}
	AnimatedText->SetDefaultColor(Text);
	AnimatedText->SetReduceMotion(bReduceMotion);
	FSlateFontInfo BodyFont = InTheme && InTheme->BodyFont.HasValidFont()
		? InTheme->BodyFont : FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 30);
	FSlateFontInfo NameFont = InTheme && InTheme->NameFont.HasValidFont()
		? InTheme->NameFont : FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 24);
	BodyFont.Size = FMath::Max(1, FMath::RoundToInt(BodyFont.Size * UIScale));
	NameFont.Size = FMath::Max(1, FMath::RoundToInt(NameFont.Size * UIScale));
	AnimatedText->SetFont(BodyFont);
	SpeakerName->SetFont(NameFont);
	PortraitBox->SetWidthOverride(220.0f * UIScale);
	PortraitBox->SetHeightOverride(250.0f * UIScale);
}

void UChopItDialogueWidget::ShowLine(const FChopItDialogueLine& Line, const FChopItDialogueMarkupDocument& Document)
{
	const UChopItDialogueSpeakerDefinition* Speaker = Line.Speaker;
	SpeakerName->SetText(Speaker ? Speaker->DisplayName : FText::GetEmpty());
	SpeakerName->SetVisibility(Speaker && !Speaker->DisplayName.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Speaker) SpeakerName->SetColorAndOpacity(FSlateColor(Speaker->AccentColor));
	ArrangePortrait(Speaker ? Speaker->PortraitSide : EChopItDialoguePortraitSide::Left);
	SetPortraitExpression(Speaker, Line.Expression, false);
	AnimatedText->SetDocument(Document);
	ChoiceBox->ClearChildren();
	ChoiceBorders.Reset();
	ChoiceBox->SetVisibility(ESlateVisibility::Collapsed);
	UpdatePanelForChoiceCount(0);
	AdvanceIndicator->SetVisibility(ESlateVisibility::Collapsed);
	ResetTransition();
}

void UChopItDialogueWidget::SetRevealCount(const int32 Count)
{
	if (AnimatedText) AnimatedText->SetRevealCount(Count);
}

void UChopItDialogueWidget::SetChoices(const TArray<FChopItDialogueChoiceView>& Choices, const int32 SelectedIndex)
{
	ChoiceBox->ClearChildren();
	ChoiceBorders.Reset();
	ChoiceBox->SetVisibility(Choices.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	UpdatePanelForChoiceCount(Choices.Num());
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
		Border->SetPadding(FMargin(12.0f, 4.0f));
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(FText::Format(FText::FromString(TEXT("{0}.  {1}")), FText::AsNumber(Index + 1), Choices[Index].Text));
		Label->SetAutoWrapText(true);
		Label->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FMath::RoundToInt(21.0f * CachedUIScale)));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.97f, 0.91f, 0.78f)));
		Border->SetContent(Label);
		ChoiceBox->AddChildToVerticalBox(Border)->SetPadding(FMargin(0.0f, 2.0f));
		ChoiceBorders.Add(Border);
	}
	SetSelectedChoice(SelectedIndex);
}

void UChopItDialogueWidget::UpdatePanelForChoiceCount(const int32 ChoiceCount)
{
	if (!SafeSlot) return;
	// Choices need their own vertical space. Expanding upward keeps the authored text
	// layout intact instead of shrinking it until Slate paints both layers together.
	const float Top = ChoiceCount > 0
		? FMath::Max(0.38f, 0.60f - 0.065f * static_cast<float>(ChoiceCount))
		: 0.60f;
	SafeSlot->SetAnchors(FAnchors(0.045f, Top, 0.955f, 0.965f));
	SafeSlot->SetOffsets(FMargin(0.0f));
}

void UChopItDialogueWidget::SetSelectedChoice(const int32 SelectedIndex)
{
	for (int32 Index = 0; Index < ChoiceBorders.Num(); ++Index) ApplyChoiceStyle(Index == SelectedIndex ? Index : -Index - 1);
}

void UChopItDialogueWidget::ApplyChoiceStyle(const int32 EncodedIndex)
{
	const bool bSelected = EncodedIndex >= 0;
	const int32 Index = bSelected ? EncodedIndex : -EncodedIndex - 1;
	if (ChoiceBorders.IsValidIndex(Index)) ChoiceBorders[Index]->SetBrushColor(bSelected ? CachedSelectedColor : CachedChoiceColor);
}

void UChopItDialogueWidget::SetPortraitExpression(const UChopItDialogueSpeakerDefinition* Speaker, const FName Expression, const bool bReact)
{
	UTexture2D* Texture = Speaker ? Speaker->ResolvePortrait(Expression) : nullptr;
	PortraitBox->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!Texture)
	{
		PreviousPortrait->SetVisibility(ESlateVisibility::Collapsed);
		PreviousPortrait->SetOpacity(0.0f);
		return;
	}

	UObject* ExistingPortrait = CurrentPortrait->GetBrush().GetResourceObject();
	const bool bCrossfade = ExistingPortrait && ExistingPortrait != Texture;
	if (bCrossfade)
	{
		PreviousPortrait->SetBrush(CurrentPortrait->GetBrush());
		PreviousPortrait->SetVisibility(ESlateVisibility::HitTestInvisible);
		PreviousPortrait->SetOpacity(1.0f);
	}
	else
	{
		// Keeping the previous transparent portrait alive underneath the new one was
		// the source of the apparent double-character/overlap artifact.
		PreviousPortrait->SetVisibility(ESlateVisibility::Collapsed);
		PreviousPortrait->SetOpacity(0.0f);
	}
	CurrentPortrait->SetBrushFromTexture(Texture, true);
	CurrentPortrait->SetOpacity(bCrossfade ? 0.0f : 1.0f);
	PortraitBlendElapsed = (bCrossfade || bReact) ? 0.0f : 1.0f;
}

void UChopItDialogueWidget::SetAwaitingAdvance(const bool bAwaiting)
{
	AdvanceIndicator->SetVisibility(bAwaiting ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UChopItDialogueWidget::SetTransitionProgress(const float Alpha, const bool bExiting, const bool bWholePanel)
{
	const float T = FMath::Clamp(Alpha, 0.0f, 1.0f);
	UWidget* Target = bWholePanel ? static_cast<UWidget*>(InnerBorder) : static_cast<UWidget*>(TextArea);
	if (!Target) return;
	if (bExiting)
	{
		Target->SetRenderOpacity(1.0f - T);
		Target->SetRenderTranslation(FVector2D(0.0f, bReduceMotion ? 0.0f : -18.0f * T));
		Target->SetRenderScale(bReduceMotion ? FVector2D(1.0f) : FVector2D(1.0f + 0.08f * T));
	}
	else
	{
		Target->SetRenderOpacity(T);
		Target->SetRenderTranslation(FVector2D(0.0f, bReduceMotion ? 0.0f : 12.0f * (1.0f - T)));
		Target->SetRenderScale(bReduceMotion ? FVector2D(1.0f) : FVector2D(0.96f + 0.04f * T));
	}
}

void UChopItDialogueWidget::ResetTransition()
{
	if (TextArea)
	{
		TextArea->SetRenderOpacity(1.0f);
		TextArea->SetRenderTranslation(FVector2D::ZeroVector);
		TextArea->SetRenderScale(FVector2D(1.0f));
	}
	if (InnerBorder)
	{
		InnerBorder->SetRenderOpacity(1.0f);
		InnerBorder->SetRenderTranslation(FVector2D::ZeroVector);
		InnerBorder->SetRenderScale(FVector2D(1.0f));
	}
}

void UChopItDialogueWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (AnimatedText)
	{
		// Desired height must use the same width Slate receives for painting. The old
		// fixed 900px estimate over-counted wrapped lines on wide panels and left a
		// large invisible spacer before choices.
		const float ActualTextWidth = AnimatedText->GetCachedGeometry().GetLocalSize().X;
		if (ActualTextWidth > 50.0f) AnimatedText->SetWrapAt(ActualTextWidth);
	}
	IndicatorTime += InDeltaTime;
	if (AdvanceIndicator && AdvanceIndicator->GetVisibility() != ESlateVisibility::Collapsed)
	{
		AdvanceIndicator->SetRenderTranslation(FVector2D(0.0f, bReduceMotion ? 0.0f : FMath::Sin(IndicatorTime * 5.0f) * 3.0f));
	}
	if (PortraitBlendElapsed < 1.0f)
	{
		PortraitBlendElapsed = FMath::Min(1.0f, PortraitBlendElapsed + InDeltaTime / 0.16f);
		const float Smooth = FMath::InterpEaseOut(0.0f, 1.0f, PortraitBlendElapsed, 2.5f);
		const bool bCrossfading = PreviousPortrait->GetVisibility() != ESlateVisibility::Collapsed;
		CurrentPortrait->SetOpacity(bCrossfading ? Smooth : 1.0f);
		PreviousPortrait->SetOpacity(bCrossfading ? 1.0f - Smooth : 0.0f);
		CurrentPortrait->SetRenderScale(bReduceMotion ? FVector2D(1.0f) : FVector2D(1.08f - 0.08f * Smooth));
		if (PortraitBlendElapsed >= 1.0f)
		{
			PreviousPortrait->SetVisibility(ESlateVisibility::Collapsed);
			PreviousPortrait->SetOpacity(0.0f);
		}
	}
	else if (CurrentPortrait)
	{
		const float Breath = bReduceMotion ? 0.0f : (0.5f + 0.5f * FMath::Sin(IndicatorTime * 1.7f));
		CurrentPortrait->SetRenderScale(FVector2D(1.0f + Breath * 0.008f));
		CurrentPortrait->SetRenderTranslation(FVector2D(0.0f, -Breath * 1.5f));
	}
}
