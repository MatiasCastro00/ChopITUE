#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct CHOPITPRESENTATION_API FChopItDialogueGlyphStyle
{
	FLinearColor Color = FLinearColor::White;
	bool bHasColor = false;
	float ShakeAmplitude = 0.0f;
	float ShakeRate = 24.0f;
	float WaveAmplitude = 0.0f;
	float WaveRate = 6.0f;
	float PulseScale = 0.0f;
	float PulseRate = 4.0f;
	float SizeScale = 1.0f;
	float SpeedScale = 1.0f;
};

struct CHOPITPRESENTATION_API FChopItDialogueGlyph
{
	FString Grapheme;
	FChopItDialogueGlyphStyle Style;
	float ExtraDelayAfter = 0.0f;
};

struct CHOPITPRESENTATION_API FChopItDialogueMarkupCue
{
	int32 GlyphIndex = 0;
	FName MarkerId;
	FGameplayTag EventTag;
	FName Face;
	FName Camera;
	FName Sound;
	FName TargetBinding;
	float PauseSeconds = 0.0f;
	bool bFireOnFastForward = true;
};

struct CHOPITPRESENTATION_API FChopItDialogueMarkupDocument
{
	TArray<FChopItDialogueGlyph> Glyphs;
	TArray<FChopItDialogueMarkupCue> Cues;
	FString PlainText;
	FString Error;
	bool bValid = true;
};

/** Small, strict and asset-safe dialogue markup compiler. */
class CHOPITPRESENTATION_API FChopItDialogueMarkup
{
public:
	static FChopItDialogueMarkupDocument Compile(const FString& Source);
};

