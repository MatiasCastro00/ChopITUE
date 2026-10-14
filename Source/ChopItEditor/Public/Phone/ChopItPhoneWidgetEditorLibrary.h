#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ChopItPhoneWidgetEditorLibrary.generated.h"

/** One-time authoring helper: fills the ring Widget Blueprint's UMG Designer tree. */
UCLASS()
class CHOPITEDITOR_API UChopItPhoneWidgetEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="ChopIt|Phone|Editor")
	static bool BuildEditablePhoneIndicator();
	UFUNCTION(BlueprintCallable, Category="ChopIt|Phone|Editor")
	static bool BuildEditablePhonePrompt();
	UFUNCTION(BlueprintCallable, Category="ChopIt|Phone|Editor")
	static bool BuildEditablePhoneDialogue();
	UFUNCTION(BlueprintCallable, Category="ChopIt|Phone|Editor")
	static bool ExportPhoneDialoguePreview();
};
