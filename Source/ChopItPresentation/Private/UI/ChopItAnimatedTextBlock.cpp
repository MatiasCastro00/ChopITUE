#include "UI/ChopItAnimatedTextBlock.h"

#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

class SChopItAnimatedText final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SChopItAnimatedText) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&) { SetCanTick(false); }

	void SetDocument(const FChopItDialogueMarkupDocument& InDocument) { Document = InDocument; Invalidate(EInvalidateWidgetReason::LayoutAndVolatility); }
	void SetRevealCount(const int32 InCount) { RevealCount = InCount; Invalidate(EInvalidateWidgetReason::PaintAndVolatility); }
	void SetReduceMotion(const bool bInReduceMotion) { bReduceMotion = bInReduceMotion; Invalidate(EInvalidateWidgetReason::PaintAndVolatility); }
	void SetDefaultColor(const FLinearColor& InColor) { DefaultColor = InColor; Invalidate(EInvalidateWidgetReason::Paint); }
	void SetFont(const FSlateFontInfo& InFont) { Font = InFont; Invalidate(EInvalidateWidgetReason::LayoutAndVolatility); }
	void SetWrapAt(const float InWrapAt) { WrapAt = InWrapAt; Invalidate(EInvalidateWidgetReason::Layout); }

private:
	struct FPositionedGlyph
	{
		FVector2D Position = FVector2D::ZeroVector;
		FVector2D Size = FVector2D::ZeroVector;
	};

	FVector2D Layout(const float AvailableWidth, TArray<FPositionedGlyph>* OutPositions = nullptr) const
	{
		if (!FSlateApplication::IsInitialized()) return FVector2D(AvailableWidth, 120.0f);
		const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		const float Width = FMath::Max(50.0f, AvailableWidth);
		const float BaseLineHeight = FMath::Max(1.0f, Measure->GetMaxCharacterHeight(Font) * 1.28f);
		float X = 0.0f;
		float Y = 0.0f;
		float LineHeight = BaseLineHeight;
		float MaxX = 0.0f;
		if (OutPositions) OutPositions->SetNum(Document.Glyphs.Num());
		auto MeasureGlyph = [&Measure, this](const FChopItDialogueGlyph& Glyph)
		{
			FSlateFontInfo MeasureFont = Font;
			MeasureFont.Size = FMath::Max(1, FMath::RoundToInt(Font.Size * Glyph.Style.SizeScale));
			return Measure->Measure(Glyph.Grapheme, MeasureFont);
		};
		auto IsBreakableWhitespace = [](const FString& Grapheme)
		{
			return !Grapheme.IsEmpty() && !Grapheme.Contains(TEXT("\n"))
				&& Grapheme.TrimStartAndEnd().IsEmpty();
		};

		for (int32 Index = 0; Index < Document.Glyphs.Num(); ++Index)
		{
			const FChopItDialogueGlyph& Glyph = Document.Glyphs[Index];
			const FVector2D GlyphSize = MeasureGlyph(Glyph);
			if (Glyph.Grapheme.Contains(TEXT("\n")))
			{
				if (OutPositions) (*OutPositions)[Index] = {FVector2D(X, Y), FVector2D::ZeroVector};
				MaxX = FMath::Max(MaxX, X);
				X = 0.0f;
				Y += LineHeight;
				LineHeight = BaseLineHeight;
				continue;
			}

			// Measure the complete upcoming word before placing its first grapheme.
			// This keeps reveal/effects per grapheme while making wrapping atomic per
			// word. A single word wider than the box intentionally overflows instead
			// of being split into misleading fragments such as "rom" / "perla".
			const bool bStartsWord = !IsBreakableWhitespace(Glyph.Grapheme)
				&& (Index == 0
					|| Document.Glyphs[Index - 1].Grapheme.Contains(TEXT("\n"))
					|| IsBreakableWhitespace(Document.Glyphs[Index - 1].Grapheme));
			if (bStartsWord)
			{
				float WordWidth = 0.0f;
				for (int32 WordIndex = Index; WordIndex < Document.Glyphs.Num(); ++WordIndex)
				{
					const FChopItDialogueGlyph& WordGlyph = Document.Glyphs[WordIndex];
					if (WordGlyph.Grapheme.Contains(TEXT("\n")) || IsBreakableWhitespace(WordGlyph.Grapheme)) break;
					WordWidth += MeasureGlyph(WordGlyph).X;
				}
				if (X > 0.0f && X + WordWidth > Width)
				{
					MaxX = FMath::Max(MaxX, X);
					X = 0.0f;
					Y += LineHeight;
					LineHeight = BaseLineHeight;
				}
			}
			if (OutPositions) (*OutPositions)[Index] = {FVector2D(X, Y), GlyphSize};
			X += GlyphSize.X;
			LineHeight = FMath::Max(LineHeight, GlyphSize.Y * 1.28f);
		}
		MaxX = FMath::Max(MaxX, X);
		return FVector2D(FMath::Min(Width, MaxX), Y + LineHeight);
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return Layout(WrapAt);
	}

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& AllottedGeometry, const FSlateRect&, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		TArray<FPositionedGlyph> Positions;
		Layout(AllottedGeometry.GetLocalSize().X, &Positions);
		const double Now = FPlatformTime::Seconds();
		const int32 VisibleCount = FMath::Clamp(RevealCount, 0, Document.Glyphs.Num());
		for (int32 Index = 0; Index < VisibleCount; ++Index)
		{
			const FChopItDialogueGlyph& Glyph = Document.Glyphs[Index];
			if (Glyph.Grapheme.Contains(TEXT("\n")) || Glyph.Grapheme.IsEmpty()) continue;
			FVector2D Offset = Positions[Index].Position;
			float AnimatedScale = 1.0f;
			if (!bReduceMotion)
			{
				if (Glyph.Style.WaveAmplitude > 0.0f) Offset.Y += FMath::Sin(Now * Glyph.Style.WaveRate + Index * 0.55) * Glyph.Style.WaveAmplitude;
				if (Glyph.Style.ShakeAmplitude > 0.0f)
				{
					Offset.X += FMath::Sin(Now * Glyph.Style.ShakeRate * 1.17 + Index * 7.13) * Glyph.Style.ShakeAmplitude;
					Offset.Y += FMath::Cos(Now * Glyph.Style.ShakeRate + Index * 4.91) * Glyph.Style.ShakeAmplitude;
				}
				if (Glyph.Style.PulseScale > 0.0f) AnimatedScale += (0.5f + 0.5f * FMath::Sin(Now * Glyph.Style.PulseRate + Index * 0.2)) * Glyph.Style.PulseScale;
			}
			FSlateFontInfo DrawFont = Font;
			DrawFont.Size = FMath::Max(1, FMath::RoundToInt(Font.Size * Glyph.Style.SizeScale * AnimatedScale));
			const FLinearColor Color = InWidgetStyle.GetColorAndOpacityTint() * (Glyph.Style.bHasColor ? Glyph.Style.Color : DefaultColor);
			const ESlateDrawEffect DrawEffect = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(Positions[Index].Size, FSlateLayoutTransform(Offset)),
				Glyph.Grapheme,
				DrawFont,
				DrawEffect,
				Color);
		}
		return LayerId;
	}

	FChopItDialogueMarkupDocument Document;
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 30);
	FLinearColor DefaultColor = FLinearColor::White;
	float WrapAt = 900.0f;
	int32 RevealCount = 0;
	bool bReduceMotion = false;
};

UChopItAnimatedTextBlock::UChopItAnimatedTextBlock()
{
	Font = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 30);
	SetVisibilityInternal(ESlateVisibility::HitTestInvisible);
}

void UChopItAnimatedTextBlock::SetDocument(const FChopItDialogueMarkupDocument& InDocument)
{
	Document = InDocument;
	RevealCount = 0;
	if (SlateText.IsValid()) { SlateText->SetDocument(Document); SlateText->SetRevealCount(0); }
}

void UChopItAnimatedTextBlock::SetRevealCount(const int32 InRevealCount)
{
	RevealCount = FMath::Clamp(InRevealCount, 0, Document.Glyphs.Num());
	if (SlateText.IsValid()) SlateText->SetRevealCount(RevealCount);
}

void UChopItAnimatedTextBlock::SetReduceMotion(const bool bInReduceMotion)
{
	bReduceMotion = bInReduceMotion;
	if (SlateText.IsValid()) SlateText->SetReduceMotion(bReduceMotion);
}

void UChopItAnimatedTextBlock::SetDefaultColor(const FLinearColor& InColor)
{
	DefaultColor = InColor;
	if (SlateText.IsValid()) SlateText->SetDefaultColor(DefaultColor);
}

void UChopItAnimatedTextBlock::SetFont(const FSlateFontInfo& InFont)
{
	Font = InFont;
	if (SlateText.IsValid()) SlateText->SetFont(Font);
}

void UChopItAnimatedTextBlock::SetWrapAt(const float InWrapAt)
{
	const float Clamped = FMath::Max(50.0f, InWrapAt);
	if (FMath::IsNearlyEqual(WrapAt, Clamped, 1.0f)) return;
	WrapAt = Clamped;
	if (SlateText.IsValid()) SlateText->SetWrapAt(WrapAt);
}

void UChopItAnimatedTextBlock::ReleaseSlateResources(const bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateText.Reset();
}

TSharedRef<SWidget> UChopItAnimatedTextBlock::RebuildWidget()
{
	SlateText = SNew(SChopItAnimatedText);
	return SlateText.ToSharedRef();
}

void UChopItAnimatedTextBlock::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (!SlateText.IsValid()) return;
	SlateText->SetDocument(Document);
	SlateText->SetRevealCount(RevealCount);
	SlateText->SetReduceMotion(bReduceMotion);
	SlateText->SetDefaultColor(DefaultColor);
	SlateText->SetFont(Font);
	SlateText->SetWrapAt(WrapAt);
}
