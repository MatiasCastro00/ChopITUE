#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Dialogue/ChopItDialogueMarkup.h"
#include "Fonts/SlateFontInfo.h"
#include "ChopItAnimatedTextBlock.generated.h"

class SChopItAnimatedText;

/** One Slate leaf for stable wrapping, grapheme reveal and per-glyph effects. */
UCLASS(BlueprintType)
class CHOPITPRESENTATION_API UChopItAnimatedTextBlock final : public UWidget
{
	GENERATED_BODY()
public:
	UChopItAnimatedTextBlock();

	void SetDocument(const FChopItDialogueMarkupDocument& InDocument);
	void SetRevealCount(int32 InRevealCount);
	void SetReduceMotion(bool bInReduceMotion);
	void SetDefaultColor(const FLinearColor& InColor);
	void SetFont(const FSlateFontInfo& InFont);
	void SetWrapAt(float InWrapAt);
	int32 GetGlyphCount() const { return Document.Glyphs.Num(); }
	const FChopItDialogueMarkupDocument& GetDocument() const { return Document; }

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;

private:
	FChopItDialogueMarkupDocument Document;
	int32 RevealCount = 0;
	bool bReduceMotion = false;

	UPROPERTY(EditAnywhere, Category="Appearance") FSlateFontInfo Font;
	UPROPERTY(EditAnywhere, Category="Appearance") FLinearColor DefaultColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, Category="Appearance", meta=(ClampMin="50.0")) float WrapAt = 900.0f;

	TSharedPtr<SChopItAnimatedText> SlateText;
};
