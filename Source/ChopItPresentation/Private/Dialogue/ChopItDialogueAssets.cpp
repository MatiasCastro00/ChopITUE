#include "Dialogue/ChopItDialogueAssets.h"
#include "Dialogue/ChopItDialogueMarkup.h"

#include "Engine/Texture2D.h"
#include "Misc/DataValidation.h"

UTexture2D* UChopItDialogueSpeakerDefinition::ResolvePortrait(const FName Expression) const
{
	const TSoftObjectPtr<UTexture2D>* Portrait = Portraits.Find(Expression);
	if (!Portrait) Portrait = Portraits.Find(NeutralExpression);
	return Portrait ? Portrait->LoadSynchronous() : nullptr;
}

FPrimaryAssetId UChopItDialogueSequence::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ChopItDialogue"), DialogueId.IsNone() ? GetFName() : DialogueId);
}

const FChopItDialogueLine* UChopItDialogueSequence::FindLine(const FName LineId) const
{
	return Lines.FindByPredicate([LineId](const FChopItDialogueLine& Line) { return Line.LineId == LineId; });
}

bool UChopItDialogueSequence::ValidateSequence(TArray<FText>& OutErrors) const
{
	TSet<FName> Ids;
	for (const FChopItDialogueLine& Line : Lines)
	{
		if (Line.LineId.IsNone()) OutErrors.Add(FText::FromString(TEXT("Dialogue line has no LineId.")));
		else if (Ids.Contains(Line.LineId)) OutErrors.Add(FText::FromString(FString::Printf(TEXT("Duplicate dialogue LineId: %s"), *Line.LineId.ToString())));
		Ids.Add(Line.LineId);
	}
	if (EntryLineId.IsNone() || !Ids.Contains(EntryLineId)) OutErrors.Add(FText::FromString(TEXT("EntryLineId does not resolve to a line.")));
	for (const FChopItDialogueLine& Line : Lines)
	{
		if (!Line.NextLineId.IsNone() && !Ids.Contains(Line.NextLineId)) OutErrors.Add(FText::FromString(FString::Printf(TEXT("Line %s targets missing line %s."), *Line.LineId.ToString(), *Line.NextLineId.ToString())));
		TSet<FName> ChoiceIds;
		for (const FChopItDialogueChoice& Choice : Line.Choices)
		{
			if (Choice.ChoiceId.IsNone() || ChoiceIds.Contains(Choice.ChoiceId)) OutErrors.Add(FText::FromString(FString::Printf(TEXT("Line %s has an empty or duplicate ChoiceId."), *Line.LineId.ToString())));
			ChoiceIds.Add(Choice.ChoiceId);
			if (!Choice.NextLineId.IsNone() && !Ids.Contains(Choice.NextLineId)) OutErrors.Add(FText::FromString(FString::Printf(TEXT("Choice %s targets missing line %s."), *Choice.ChoiceId.ToString(), *Choice.NextLineId.ToString())));
		}
		const FChopItDialogueMarkupDocument Markup = FChopItDialogueMarkup::Compile(Line.Text.ToString());
		if (!Markup.bValid)
		{
			OutErrors.Add(FText::FromString(FString::Printf(
				TEXT("Line %s has invalid dialogue markup: %s"),
				*Line.LineId.ToString(), *Markup.Error)));
		}
	}

	if (Ids.Contains(EntryLineId))
	{
		TSet<FName> Reachable;
		TArray<FName> Pending = {EntryLineId};
		while (!Pending.IsEmpty())
		{
			const FName CurrentId = Pending.Pop(EAllowShrinking::No);
			if (Reachable.Contains(CurrentId)) continue;
			Reachable.Add(CurrentId);
			const FChopItDialogueLine* CurrentLine = FindLine(CurrentId);
			if (!CurrentLine) continue;
			if (!CurrentLine->NextLineId.IsNone() && Ids.Contains(CurrentLine->NextLineId)) Pending.Add(CurrentLine->NextLineId);
			for (const FChopItDialogueChoice& Choice : CurrentLine->Choices)
			{
				if (!Choice.NextLineId.IsNone() && Ids.Contains(Choice.NextLineId)) Pending.Add(Choice.NextLineId);
			}
		}
		for (const FName Id : Ids)
		{
			if (!Reachable.Contains(Id)) OutErrors.Add(FText::FromString(FString::Printf(TEXT("Dialogue line is unreachable: %s"), *Id.ToString())));
		}
	}
	return OutErrors.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UChopItDialogueSequence::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);
	TArray<FText> Errors;
	if (!ValidateSequence(Errors))
	{
		for (const FText& Error : Errors) Context.AddError(Error);
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif
