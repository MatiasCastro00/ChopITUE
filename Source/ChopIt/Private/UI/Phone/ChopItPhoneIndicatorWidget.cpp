#include "UI/Phone/ChopItPhoneIndicatorWidget.h"

#include "UI/Phone/ChopItPhoneIndicatorViewModel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UChopItPhoneIndicatorWidget::SetArtwork(UTexture2D* InBubbleTexture, UTexture2D* InRingTexture)
{
	BubbleTexture = InBubbleTexture;
	RingTexture = InRingTexture;
	if (BubbleImage && BubbleTexture) BubbleImage->SetBrushFromTexture(BubbleTexture);
	if (RingLeftImage && RingTexture) RingLeftImage->SetBrushFromTexture(RingTexture);
	if (RingRightImage && RingTexture) RingRightImage->SetBrushFromTexture(RingTexture);
	ApplyViewModel();
}

void UChopItPhoneIndicatorWidget::SetViewModel(UChopItPhoneIndicatorViewModel* InViewModel)
{
	if (ViewModel) ViewModel->RemoveAllFieldValueChangedDelegates(this);
	ViewModel = InViewModel;
	if (ViewModel)
	{
		using FDescriptor = UChopItPhoneIndicatorViewModel::FFieldNotificationClassDescriptor;
		const auto Bind = [this](UE::FieldNotification::FFieldId Field)
		{
			ViewModel->AddFieldValueChangedDelegate(Field,
				UChopItPhoneIndicatorViewModel::FFieldValueChangedDelegate::CreateUObject(this, &UChopItPhoneIndicatorWidget::OnFieldChanged));
		};
		Bind(FDescriptor::Label);
		Bind(FDescriptor::ScreenPosition);
		Bind(FDescriptor::Scale);
		Bind(FDescriptor::Opacity);
		Bind(FDescriptor::ArrowDegrees);
		Bind(FDescriptor::bVisible);
		Bind(FDescriptor::bOffscreen);
		Bind(FDescriptor::RingPulse);
		Bind(FDescriptor::TiltDegrees);
	}
	ApplyViewModel();
}

TSharedRef<SWidget> UChopItPhoneIndicatorWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) BuildWidgetTree();
	// Designer-authored widget trees do not pass through BuildWidgetTree(). Keep the
	// projected phone point at the comic's center for every render scale.
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	if (WidgetTree && WidgetTree->RootWidget)
	{
		BubbleImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("ComicBubble")));
		RingLeftImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("RingLeft")));
		RingRightImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("RingRight")));
		ArrowText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("DirectionArrow")));
		LabelText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("RingText")));
		if (BubbleImage && BubbleTexture) BubbleImage->SetBrushFromTexture(BubbleTexture);
		if (RingLeftImage && RingTexture) RingLeftImage->SetBrushFromTexture(RingTexture);
		if (RingRightImage && RingTexture) RingRightImage->SetBrushFromTexture(RingTexture);
	}
	const TSharedRef<SWidget> Built = Super::RebuildWidget();
	UE_LOG(LogTemp, Display, TEXT("Phone indicator widget built: root=%s bubble=%s rings=%s/%s."),
		*GetNameSafe(WidgetTree ? WidgetTree->RootWidget : nullptr),
		*GetNameSafe(BubbleImage), *GetNameSafe(RingLeftImage), *GetNameSafe(RingRightImage));
	ApplyViewModel();
	return Built;
}

void UChopItPhoneIndicatorWidget::BuildWidgetTree()
{
	USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ComicFrame"));
	Frame->SetWidthOverride(240.0f);
	Frame->SetHeightOverride(150.0f);
	WidgetTree->RootWidget = Frame;
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ComicCanvas"));
	Frame->AddChild(Canvas);

	BubbleImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ComicBubble"));
	if (BubbleTexture) BubbleImage->SetBrushFromTexture(BubbleTexture);
	BubbleImage->SetColorAndOpacity(FLinearColor(1.0f, 0.97f, 0.78f, 1.0f));
	UCanvasPanelSlot* BubbleSlot = Canvas->AddChildToCanvas(BubbleImage);
	BubbleSlot->SetPosition(FVector2D(7.5f, 0));
	BubbleSlot->SetSize(FVector2D(225, 150));
	BubbleSlot->SetZOrder(0);

	RingLeftImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RingLeft"));
	if (RingTexture) RingLeftImage->SetBrushFromTexture(RingTexture);
	RingLeftImage->SetColorAndOpacity(FLinearColor(1.0f, 0.80f, 0.28f, 1.0f));
	UCanvasPanelSlot* LeftSlot = Canvas->AddChildToCanvas(RingLeftImage);
	LeftSlot->SetPosition(FVector2D(15, 10));
	LeftSlot->SetSize(FVector2D(118, 70));
	LeftSlot->SetZOrder(1);
	RingLeftImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	RingRightImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RingRight"));
	if (RingTexture) RingRightImage->SetBrushFromTexture(RingTexture);
	RingRightImage->SetColorAndOpacity(FLinearColor(1.0f, 0.80f, 0.28f, 1.0f));
	UCanvasPanelSlot* RightSlot = Canvas->AddChildToCanvas(RingRightImage);
	RightSlot->SetPosition(FVector2D(102, 68));
	RightSlot->SetSize(FVector2D(108, 64));
	RightSlot->SetZOrder(2);
	RingRightImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	ArrowText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DirectionArrow"));
	ArrowText->SetText(FText::FromString(TEXT("➤")));
	FSlateFontInfo ArrowFont = ArrowText->GetFont();
	ArrowFont.Size = 19;
	ArrowText->SetFont(ArrowFont);
	ArrowText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	UCanvasPanelSlot* ArrowSlot = Canvas->AddChildToCanvas(ArrowText);
	ArrowSlot->SetPosition(FVector2D(111, 128));
	ArrowSlot->SetSize(FVector2D(23, 21));
	ArrowSlot->SetZOrder(3);
	ArrowText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RingText"));
	FSlateFontInfo LabelFont = LabelText->GetFont();
	LabelFont.Size = 16;
	LabelText->SetFont(LabelFont);
	LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	UCanvasPanelSlot* LabelSlot = Canvas->AddChildToCanvas(LabelText);
	LabelSlot->SetPosition(FVector2D(57, 60));
	LabelSlot->SetSize(FVector2D(160, 30));
	LabelSlot->SetZOrder(3);
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	SetVisibility(ESlateVisibility::Hidden);
}

void UChopItPhoneIndicatorWidget::OnFieldChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId)
{
	if (Object == ViewModel) ApplyViewModel();
}

void UChopItPhoneIndicatorWidget::ApplyViewModel()
{
	if (!ViewModel || !BubbleImage || !RingLeftImage || !RingRightImage || !LabelText || !ArrowText) return;
	LabelText->SetText(ViewModel->Label);
	const bool bShowSprites = BubbleTexture && RingTexture;
	BubbleImage->SetVisibility(bShowSprites ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	RingLeftImage->SetVisibility(bShowSprites ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	RingRightImage->SetVisibility(bShowSprites ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	LabelText->SetVisibility(bShowSprites ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	const float Pulse = ViewModel->RingPulse;
	BubbleImage->SetRenderScale(FVector2D(1.0f + 0.03f * FMath::Abs(Pulse)));
	RingLeftImage->SetRenderTransformAngle(-5.0f + Pulse * 7.0f);
	RingRightImage->SetRenderTransformAngle(5.0f - Pulse * 7.0f);
	RingLeftImage->SetRenderScale(FVector2D(1.0f + FMath::Abs(Pulse) * 0.035f));
	RingRightImage->SetRenderScale(FVector2D(1.0f + FMath::Abs(Pulse) * 0.035f));
	ArrowText->SetVisibility(ViewModel->bOffscreen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	ArrowText->SetRenderTransformAngle(ViewModel->ArrowDegrees);
	SetPositionInViewport(ViewModel->ScreenPosition, true);
	SetRenderScale(FVector2D(ViewModel->Scale));
	SetRenderTransformAngle(ViewModel->TiltDegrees);
	SetRenderOpacity(ViewModel->Opacity);
	SetVisibility(ViewModel->bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

void UChopItPhoneIndicatorWidget::NativeDestruct()
{
	if (ViewModel) ViewModel->RemoveAllFieldValueChangedDelegates(this);
	Super::NativeDestruct();
}
