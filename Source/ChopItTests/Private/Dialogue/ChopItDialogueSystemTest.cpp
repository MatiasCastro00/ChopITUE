#include "Dialogue/ChopItDialogueAssets.h"
#include "Dialogue/ChopItDialogueMarkup.h"
#include "Camera/ChopItCameraComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItDialogueMarkupTest,
	"ChopIt.Dialogue.Markup.NestingUnicodeAndCues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItDialogueMarkupTest::RunTest(const FString&)
{
	const FChopItDialogueMarkupDocument Document = FChopItDialogueMarkup::Compile(
		TEXT("A<shake amp=\"3\"><wave amp=\"4\"><color value=\"#FF6A00\">á🌲</color></wave></shake><cue event=\"Dialogue.Event.Warning\" face=\"Angry\" camera=\"Impact\"/>"));
	TestTrue(TEXT("Nested markup compiles"), Document.bValid);
	TestEqual(TEXT("Unicode graphemes remain whole"), Document.Glyphs.Num(), 3);
	if (Document.Glyphs.Num() == 3)
	{
		TestEqual(TEXT("Nested shake is applied"), Document.Glyphs[1].Style.ShakeAmplitude, 3.0f);
		TestEqual(TEXT("Nested wave is applied"), Document.Glyphs[1].Style.WaveAmplitude, 4.0f);
		TestTrue(TEXT("Nested color is applied"), Document.Glyphs[1].Style.bHasColor);
	}
	TestEqual(TEXT("One semantic cue is emitted"), Document.Cues.Num(), 1);
	if (Document.Cues.Num() == 1)
	{
		TestEqual(TEXT("Cue is positioned after visible text"), Document.Cues[0].GlyphIndex, 3);
		TestEqual(TEXT("Cue changes portrait expression"), Document.Cues[0].Face, FName(TEXT("Angry")));
		TestEqual(TEXT("Cue resolves a camera action id"), Document.Cues[0].Camera, FName(TEXT("Impact")));
		TestTrue(TEXT("Cue resolves registered gameplay event"), Document.Cues[0].EventTag.IsValid());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItDialogueMarkupFallbackTest,
	"ChopIt.Dialogue.Markup.MalformedFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItDialogueMarkupFallbackTest::RunTest(const FString&)
{
	const FChopItDialogueMarkupDocument Document = FChopItDialogueMarkup::Compile(TEXT("Antes <shake>roto</wave> después"));
	TestFalse(TEXT("Mismatched markup is reported"), Document.bValid);
	TestTrue(TEXT("Fallback still contains readable text"), !Document.Glyphs.IsEmpty());
	TestEqual(TEXT("Fallback strips unsafe markup"), Document.PlainText, FString(TEXT("Antes roto después")));
	TestTrue(TEXT("Fallback carries a useful diagnostic"), !Document.Error.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItDialogueGraphValidationTest,
	"ChopIt.Dialogue.Data.GraphAndChoiceFilters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItDialogueGraphValidationTest::RunTest(const FString&)
{
	UChopItDialogueSequence* Sequence = NewObject<UChopItDialogueSequence>();
	Sequence->DialogueId = TEXT("TestDialogue");
	Sequence->EntryLineId = TEXT("Start");
	FChopItDialogueLine& Start = Sequence->Lines.AddDefaulted_GetRef();
	Start.LineId = TEXT("Start");
	Start.NextLineId = TEXT("End");
	Sequence->Lines.AddDefaulted_GetRef().LineId = TEXT("End");
	TArray<FText> Errors;
	TestTrue(TEXT("Valid graph passes"), Sequence->ValidateSequence(Errors));

	Start.Choices.AddDefaulted();
	Start.Choices[0].ChoiceId = TEXT("Broken");
	Start.Choices[0].NextLineId = TEXT("Missing");
	Errors.Reset();
	TestFalse(TEXT("Missing branch targets fail validation"), Sequence->ValidateSequence(Errors));
	Start.Choices.Reset();
	Sequence->Lines.AddDefaulted_GetRef().LineId = TEXT("Orphan");
	Errors.Reset();
	TestFalse(TEXT("Unreachable nodes fail validation"), Sequence->ValidateSequence(Errors));

	FChopItDialogueChoice Filtered;
	Filtered.RequiredTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Dialogue.Event.Warning")));
	FGameplayTagContainer Context;
	TestFalse(TEXT("Required tags hide a choice"), Filtered.IsAvailable(Context));
	Context.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Dialogue.Event.Warning")));
	TestTrue(TEXT("Satisfied tags expose a choice"), Filtered.IsAvailable(Context));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItDialogueCameraLockTest,
	"ChopIt.Dialogue.Integration.ExternalInputLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItDialogueCameraLockTest::RunTest(const FString&)
{
	UChopItCameraComponent* Camera = NewObject<UChopItCameraComponent>();
	const FChopItCameraHandle Handle = Camera->PushInputLock(EChopItCameraInputLock::Movement | EChopItCameraInputLock::Actions);
	TestTrue(TEXT("Input lock handle is valid"), Handle.IsValid());
	TestTrue(TEXT("Dialogue locks movement"), Camera->IsInputLocked(EChopItCameraInputLock::Movement));
	TestTrue(TEXT("Dialogue locks actions"), Camera->IsInputLocked(EChopItCameraInputLock::Actions));
	Camera->PopInputLock(Handle);
	TestFalse(TEXT("Popping restores movement input"), Camera->IsInputLocked(EChopItCameraInputLock::Movement));
	Camera->PopInputLock(Handle);
	TestFalse(TEXT("Repeated pop is safe"), Camera->IsInputLocked(EChopItCameraInputLock::Actions));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItDialogueDemoAssetsTest,
	"ChopIt.Dialogue.Content.DemoAssetsLoadAndValidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItDialogueDemoAssetsTest::RunTest(const FString&)
{
	UChopItDialogueSequence* Sequence = LoadObject<UChopItDialogueSequence>(
		nullptr, TEXT("/Game/ChopIt/Dialogue/Sequences/DA_Dialogue_Demo.DA_Dialogue_Demo"));
	TestNotNull(TEXT("Demo sequence loads"), Sequence);
	if (!Sequence) return false;

	TArray<FText> Errors;
	TestTrue(TEXT("Demo graph validates"), Sequence->ValidateSequence(Errors));
	TestNotNull(TEXT("Demo theme loads"), Sequence->Theme.Get());
	TestTrue(TEXT("Demo has a branching choice"), Sequence->Lines.ContainsByPredicate(
		[](const FChopItDialogueLine& Line) { return Line.Choices.Num() >= 2; }));

	TSet<const UChopItDialogueSpeakerDefinition*> Speakers;
	for (const FChopItDialogueLine& Line : Sequence->Lines)
	{
		if (Line.Speaker) Speakers.Add(Line.Speaker);
	}
	TestEqual(TEXT("Two demo speakers load"), Speakers.Num(), 2);
	for (const UChopItDialogueSpeakerDefinition* Speaker : Speakers)
	{
		TestNotNull(TEXT("Neutral portrait resolves"), Speaker->ResolvePortrait(TEXT("Neutral")));
		TestNotNull(TEXT("Angry portrait resolves"), Speaker->ResolvePortrait(TEXT("Angry")));
		TestNotNull(TEXT("Surprised portrait resolves"), Speaker->ResolvePortrait(TEXT("Surprised")));
		TestNotNull(TEXT("Unknown expression falls back to neutral"), Speaker->ResolvePortrait(TEXT("MissingExpression")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItDialogueMatchIntroAssetsTest,
	"ChopIt.Dialogue.Content.MatchIntroQuotaPortraitsAndCameras",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItDialogueMatchIntroAssetsTest::RunTest(const FString&)
{
	UChopItDialogueSequence* Sequence = LoadObject<UChopItDialogueSequence>(
		nullptr, TEXT("/Game/ChopIt/Dialogue/Sequences/DA_Dialogue_MatchIntro.DA_Dialogue_MatchIntro"));
	TestNotNull(TEXT("Match introduction sequence loads"), Sequence);
	if (!Sequence) return false;

	TArray<FText> Errors;
	TestTrue(TEXT("Match introduction graph validates"), Sequence->ValidateSequence(Errors));
	TestTrue(TEXT("Introduction includes accept, defy and question responses"), Sequence->Lines.ContainsByPredicate(
		[](const FChopItDialogueLine& Line) { return Line.Choices.Num() == 3; }));
	TestTrue(TEXT("Quota is runtime-formatted in spoken text"), Sequence->Lines.ContainsByPredicate(
		[](const FChopItDialogueLine& Line) { return Line.Text.ToString().Contains(TEXT("{Quota}")); }));

	TSet<const UChopItDialogueSpeakerDefinition*> Speakers;
	TSet<FString> SpeakerNames;
	TSet<FName> CameraActions;
	FString SpokenEnglish;
	for (const FChopItDialogueLine& Line : Sequence->Lines)
	{
		if (Line.Speaker)
		{
			Speakers.Add(Line.Speaker);
			SpeakerNames.Add(Line.Speaker->DisplayName.ToString());
		}
		if (!Line.CameraAction.IsNone()) CameraActions.Add(Line.CameraAction);
		SpokenEnglish += Line.Text.ToString();
		for (const FChopItDialogueChoice& Choice : Line.Choices) SpokenEnglish += Choice.Text.ToString();
	}
	TestEqual(TEXT("Death, Oven and Protagonist speakers load"), Speakers.Num(), 3);
	TestTrue(TEXT("Death speaker is localized to English"), SpeakerNames.Contains(TEXT("DEATH")));
	TestTrue(TEXT("Oven speaker is localized to English"), SpeakerNames.Contains(TEXT("THE OVEN")));
	TestTrue(TEXT("Protagonist speaker is localized to English"), SpeakerNames.Contains(TEXT("YOU")));
	TestTrue(TEXT("Opening dialogue is English"), SpokenEnglish.Contains(TEXT("Wake up")));
	TestTrue(TEXT("Oven threat is English"), SpokenEnglish.Contains(TEXT("I WILL DEVOUR YOU")));
	TestTrue(TEXT("Dialogue choices are English"), SpokenEnglish.Contains(TEXT("ACCEPT")));
	TestFalse(TEXT("No inverted Spanish punctuation remains in the introduction"),
		SpokenEnglish.Contains(TEXT("¿")) || SpokenEnglish.Contains(TEXT("¡")));
	for (const UChopItDialogueSpeakerDefinition* Speaker : Speakers)
	{
		TestNotNull(TEXT("Each intro speaker resolves its fallback portrait"), Speaker->ResolvePortrait(NAME_None));
	}
	TestTrue(TEXT("Camera alternates to Death"), CameraActions.Contains(TEXT("DeathCloseup")) || CameraActions.Contains(TEXT("DeathZoomIn")) || CameraActions.Contains(TEXT("DeathZoomOut")));
	TestTrue(TEXT("Camera alternates to Oven"), CameraActions.Contains(TEXT("OvenCloseup")) || CameraActions.Contains(TEXT("OvenZoomIn")) || CameraActions.Contains(TEXT("OvenZoomOut")));
	TestTrue(TEXT("Camera alternates to Protagonist"), CameraActions.Contains(TEXT("PlayerCloseup")) || CameraActions.Contains(TEXT("PlayerZoomIn")) || CameraActions.Contains(TEXT("PlayerZoomOut")));
	TestTrue(TEXT("Camera includes a chain reveal wide shot"), CameraActions.Contains(TEXT("IntroWide")));
	TestTrue(TEXT("Camera includes an authored zoom in"), CameraActions.Contains(TEXT("DeathZoomIn")) || CameraActions.Contains(TEXT("OvenZoomIn")));
	TestTrue(TEXT("Camera includes an authored zoom out"), CameraActions.Contains(TEXT("DeathZoomOut")) || CameraActions.Contains(TEXT("PlayerZoomOut")));
	TestNotNull(TEXT("Introduction theme loads"), Sequence->Theme.Get());
	if (Sequence->Theme)
	{
		const FChopItDialogueCameraAction* OvenImpact = Sequence->Theme->CameraActions.Find(TEXT("OvenImpact"));
		const FChopItDialogueCameraAction* DevourZoom = Sequence->Theme->CameraActions.Find(TEXT("OvenDevourZoom"));
		TestTrue(TEXT("Oven impact sustains until the line advances"), OvenImpact && OvenImpact->bSustainUntilLineEnds);
		TestTrue(TEXT("Devour beat has an aggressive close FOV"), DevourZoom && DevourZoom->FieldOfViewOverride <= 28.0f);
		TestTrue(TEXT("Devour beat uses a snap transition"), DevourZoom && DevourZoom->BlendInTimeOverride > 0.0f && DevourZoom->BlendInTimeOverride <= 0.06f);
	}
	const FChopItDialogueLine* OvenThreat = Sequence->FindLine(TEXT("OvenThreat"));
	TestTrue(TEXT("Chain threat begins on the wider oven shot"), OvenThreat && OvenThreat->CameraAction == TEXT("OvenZoomOut"));
	TestTrue(TEXT("Devour text triggers both snap zoom and sustained shake"), OvenThreat
		&& OvenThreat->Text.ToString().Contains(TEXT("OvenDevourZoom"))
		&& OvenThreat->Text.ToString().Contains(TEXT("OvenImpact")));
	return true;
}

#endif
