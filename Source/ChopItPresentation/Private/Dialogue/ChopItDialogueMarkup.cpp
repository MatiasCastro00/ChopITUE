#include "Dialogue/ChopItDialogueMarkup.h"

#include "Internationalization/BreakIterator.h"
#include "Internationalization/IBreakIterator.h"
#include "Internationalization/Regex.h"

namespace ChopItDialogueMarkup
{
	using FAttributes = TMap<FString, FString>;

	struct FStackEntry
	{
		FString Tag;
		FChopItDialogueGlyphStyle Style;
	};

	FAttributes ParseAttributes(const FString& Source)
	{
		FAttributes Result;
		const FRegexPattern Pattern(TEXT("([A-Za-z][A-Za-z0-9_]*)\\s*=\\s*\"([^\"]*)\""));
		FRegexMatcher Matcher(Pattern, Source);
		while (Matcher.FindNext()) Result.Add(Matcher.GetCaptureGroup(1).ToLower(), Matcher.GetCaptureGroup(2));
		return Result;
	}

	float Number(const FAttributes& Attributes, const TCHAR* Key, const float Default)
	{
		if (const FString* Value = Attributes.Find(Key)) return FCString::Atof(**Value);
		return Default;
	}

	bool Boolean(const FAttributes& Attributes, const TCHAR* Key, const bool Default)
	{
		if (const FString* Value = Attributes.Find(Key)) return !Value->Equals(TEXT("false"), ESearchCase::IgnoreCase) && *Value != TEXT("0");
		return Default;
	}

	FName Name(const FAttributes& Attributes, const TCHAR* Key)
	{
		if (const FString* Value = Attributes.Find(Key)) return FName(**Value);
		return NAME_None;
	}

	FGameplayTag Tag(const FAttributes& Attributes, const TCHAR* Key)
	{
		if (const FString* Value = Attributes.Find(Key)) return FGameplayTag::RequestGameplayTag(FName(**Value), false);
		return {};
	}

	FString DecodeEntities(FString Text)
	{
		Text.ReplaceInline(TEXT("&lt;"), TEXT("<"));
		Text.ReplaceInline(TEXT("&gt;"), TEXT(">"));
		Text.ReplaceInline(TEXT("&quot;"), TEXT("\""));
		Text.ReplaceInline(TEXT("&amp;"), TEXT("&"));
		return Text;
	}

	FString StripMarkupForFallback(const FString& Source)
	{
		FString Plain;
		Plain.Reserve(Source.Len());
		bool bInsideTag = false;
		for (const TCHAR Character : Source)
		{
			if (!bInsideTag && Character == TEXT('<')) { bInsideTag = true; continue; }
			if (bInsideTag)
			{
				if (Character == TEXT('>')) bInsideTag = false;
				continue;
			}
			Plain.AppendChar(Character);
		}
		return DecodeEntities(Plain);
	}

	void AddText(FChopItDialogueMarkupDocument& Document, const FString& RawText, const FChopItDialogueGlyphStyle& Style)
	{
		const FString Text = DecodeEntities(RawText);
		Document.PlainText += Text;
		if (Text.IsEmpty()) return;

		TSharedRef<IBreakIterator> Iterator = FBreakIterator::CreateCharacterBoundaryIterator();
		Iterator->SetStringRef(Text);
		int32 Start = Iterator->ResetToBeginning();
		for (int32 End = Iterator->MoveToNext(); End != INDEX_NONE; Start = End, End = Iterator->MoveToNext())
		{
			FChopItDialogueGlyph& Glyph = Document.Glyphs.AddDefaulted_GetRef();
			Glyph.Grapheme = Text.Mid(Start, End - Start);
			Glyph.Style = Style;
			if (Glyph.Grapheme == TEXT(",") || Glyph.Grapheme == TEXT(";") || Glyph.Grapheme == TEXT(":")) Glyph.ExtraDelayAfter = 0.075f;
			else if (Glyph.Grapheme == TEXT(".") || Glyph.Grapheme == TEXT("!") || Glyph.Grapheme == TEXT("?")) Glyph.ExtraDelayAfter = 0.16f;
			else if (Glyph.Grapheme.Contains(TEXT("\n"))) Glyph.ExtraDelayAfter = 0.12f;
		}
	}

	void AddCue(FChopItDialogueMarkupDocument& Document, const FAttributes& Attributes, const FString& SourceTag)
	{
		FChopItDialogueMarkupCue& Cue = Document.Cues.AddDefaulted_GetRef();
		Cue.GlyphIndex = Document.Glyphs.Num();
		Cue.MarkerId = Name(Attributes, TEXT("id"));
		Cue.EventTag = Tag(Attributes, TEXT("event"));
		Cue.Face = Name(Attributes, TEXT("face"));
		Cue.Camera = Name(Attributes, TEXT("camera"));
		Cue.Sound = Name(Attributes, TEXT("sfx"));
		Cue.TargetBinding = Name(Attributes, TEXT("target"));
		Cue.PauseSeconds = Number(Attributes, TEXT("seconds"), Number(Attributes, TEXT("value"), 0.0f));
		Cue.bFireOnFastForward = Boolean(Attributes, TEXT("fireonfastforward"), true);

		if (SourceTag == TEXT("face")) Cue.Face = Name(Attributes, TEXT("id"));
		else if (SourceTag == TEXT("camera")) Cue.Camera = Name(Attributes, TEXT("id"));
		else if (SourceTag == TEXT("sfx")) Cue.Sound = Name(Attributes, TEXT("id"));
		else if (SourceTag == TEXT("event"))
		{
			Cue.EventTag = Tag(Attributes, TEXT("tag"));
			if (Cue.MarkerId.IsNone()) Cue.MarkerId = Name(Attributes, TEXT("id"));
		}
	}
}

FChopItDialogueMarkupDocument FChopItDialogueMarkup::Compile(const FString& Source)
{
	using namespace ChopItDialogueMarkup;
	FChopItDialogueMarkupDocument Document;
	TArray<FStackEntry> Stack;
	FChopItDialogueGlyphStyle Current;

	int32 Cursor = 0;
	while (Cursor < Source.Len())
	{
		const int32 Open = Source.Find(TEXT("<"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor);
		if (Open == INDEX_NONE)
		{
			AddText(Document, Source.Mid(Cursor), Current);
			break;
		}
		AddText(Document, Source.Mid(Cursor, Open - Cursor), Current);
		const int32 Close = Source.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Open + 1);
		if (Close == INDEX_NONE)
		{
			Document.bValid = false;
			Document.Error = TEXT("Unclosed markup tag.");
			break;
		}

		FString Token = Source.Mid(Open + 1, Close - Open - 1).TrimStartAndEnd();
		const bool bClosing = Token.StartsWith(TEXT("/"));
		const bool bSelfClosing = Token.EndsWith(TEXT("/"));
		if (bClosing) Token.RightChopInline(1);
		if (bSelfClosing) Token.LeftChopInline(1);
		Token.TrimStartAndEndInline();
		FString TagName;
		FString AttributeText;
		if (!Token.Split(TEXT(" "), &TagName, &AttributeText)) TagName = Token;
		TagName = TagName.ToLower();

		if (bClosing)
		{
			if (Stack.IsEmpty() || Stack.Last().Tag != TagName)
			{
				Document.bValid = false;
				Document.Error = FString::Printf(TEXT("Mismatched closing tag: %s."), *TagName);
				break;
			}
			Current = Stack.Pop().Style;
			Cursor = Close + 1;
			continue;
		}

		const FAttributes Attributes = ParseAttributes(AttributeText);
		const bool bCommand = TagName == TEXT("cue") || TagName == TEXT("pause") || TagName == TEXT("face") || TagName == TEXT("event") || TagName == TEXT("camera") || TagName == TEXT("sfx");
		if (bCommand) AddCue(Document, Attributes, TagName);

		if (!bSelfClosing)
		{
			if (TagName != TEXT("shake") && TagName != TEXT("wave") && TagName != TEXT("color") && TagName != TEXT("pulse") && TagName != TEXT("size") && TagName != TEXT("speed") && TagName != TEXT("cue"))
			{
				Document.bValid = false;
				Document.Error = FString::Printf(TEXT("Unknown or non-pairable tag: %s."), *TagName);
				break;
			}
			Stack.Add({TagName, Current});
			if (TagName == TEXT("shake")) { Current.ShakeAmplitude += Number(Attributes, TEXT("amp"), 2.0f); Current.ShakeRate = Number(Attributes, TEXT("rate"), 24.0f); }
			else if (TagName == TEXT("wave")) { Current.WaveAmplitude += Number(Attributes, TEXT("amp"), 5.0f); Current.WaveRate = Number(Attributes, TEXT("rate"), 6.0f); }
			else if (TagName == TEXT("pulse")) { Current.PulseScale += Number(Attributes, TEXT("scale"), 0.12f); Current.PulseRate = Number(Attributes, TEXT("rate"), 4.0f); }
			else if (TagName == TEXT("size")) Current.SizeScale *= FMath::Max(0.1f, Number(Attributes, TEXT("value"), 1.0f));
			else if (TagName == TEXT("speed")) Current.SpeedScale *= FMath::Max(0.05f, Number(Attributes, TEXT("value"), 1.0f));
			else if (TagName == TEXT("color"))
			{
				if (const FString* Value = Attributes.Find(TEXT("value")))
				{
					Current.Color = FLinearColor(FColor::FromHex(*Value));
					Current.bHasColor = true;
				}
			}
		}
		else if (!bCommand)
		{
			Document.bValid = false;
			Document.Error = FString::Printf(TEXT("Tag %s cannot be self-closing."), *TagName);
			break;
		}
		Cursor = Close + 1;
	}

	if (Document.bValid && !Stack.IsEmpty())
	{
		Document.bValid = false;
		Document.Error = FString::Printf(TEXT("Unclosed markup tag: %s."), *Stack.Last().Tag);
	}
	if (!Document.bValid)
	{
		Document.Glyphs.Reset();
		Document.Cues.Reset();
		Document.PlainText.Reset();
		FChopItDialogueGlyphStyle PlainStyle;
		AddText(Document, StripMarkupForFallback(Source), PlainStyle);
	}
	return Document;
}
