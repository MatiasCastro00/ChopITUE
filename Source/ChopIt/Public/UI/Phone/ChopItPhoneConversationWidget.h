#pragma once

#include "UI/ChopItDialogueWidget.h"
#include "ChopItPhoneConversationWidget.generated.h"

class UHorizontalBox;
class UButton;

/** Behavior for the authored phone dialogue. All visual children live in its WBP. */
UCLASS(Blueprintable)
class CHOPIT_API UChopItPhoneConversationWidget : public UChopItDialogueWidget
{
	GENERATED_BODY()
public:
	TDelegate<bool()> OnRefreshRequested;
	TDelegate<bool()> OnSkipRequested;
	TDelegate<void(int32)> OnChoiceRequested;
	TDelegate<void(int32)> OnChoiceHovered;
	virtual void ConfigureTheme(const UChopItDialogueTheme* Theme, bool ReduceMotion, float UIScale) override;
	virtual void ShowLine(const FChopItDialogueLine& Line, const FChopItDialogueMarkupDocument& Document) override;
	virtual void SetRevealCount(int32 Count) override;
	virtual void SetChoices(const TArray<FChopItDialogueChoiceView>& Choices, int32 SelectedIndex) override;
	virtual void SetSelectedChoice(int32 SelectedIndex) override;
	virtual void SetPortraitExpression(const UChopItDialogueSpeakerDefinition*, FName, bool) override {}
	virtual void SetAwaitingAdvance(bool Awaiting) override;
	virtual void SetTransitionProgress(float Alpha, bool Exiting, bool WholePanel) override;
	virtual void ResetTransition() override;
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	UFUNCTION() void ClickRefresh();
	UFUNCTION() void ClickSkip();
	UFUNCTION() void ClickChoiceOne();
	UFUNCTION() void ClickChoiceTwo();
	UFUNCTION() void ClickChoiceThree();
	UFUNCTION() void HoverChoiceOne();
	UFUNCTION() void HoverChoiceTwo();
	UFUNCTION() void HoverChoiceThree();
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UBorder> SpeechPanel;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UChopItAnimatedTextBlock> BodyText;
	UPROPERTY(Transient, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> CallerName;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UTextBlock> ContinueHint;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UHorizontalBox> ResponseCards;
	UPROPERTY(Transient, meta=(BindWidgetOptional)) TObjectPtr<UButton> RefreshButton;
	UPROPERTY(Transient, meta=(BindWidgetOptional)) TObjectPtr<UButton> SkipButton;
	UPROPERTY(Transient, meta=(BindWidgetOptional)) TObjectPtr<UButton> ChoiceButton1;
	UPROPERTY(Transient, meta=(BindWidgetOptional)) TObjectPtr<UButton> ChoiceButton2;
	UPROPERTY(Transient, meta=(BindWidgetOptional)) TObjectPtr<UButton> ChoiceButton3;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UBorder> Card1;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UBorder> Card2;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UBorder> Card3;
	UPROPERTY(Transient, meta=(BindWidgetOptional)) TObjectPtr<UBorder> Card4;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UTextBlock> Response1;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UTextBlock> Response2;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UTextBlock> Response3;
	UPROPERTY(Transient, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Response4;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UTextBlock> CardHint1;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UTextBlock> CardHint2;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UTextBlock> CardHint3;
	UPROPERTY(Transient, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> CardHint4;
	UPROPERTY(Transient, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> CardCost1;
	UPROPERTY(Transient, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> CardCost2;
	UPROPERTY(Transient, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> CardCost3;
private:
	bool bStill = false;
	float ChoicesElapsed = 0.0f;
	int32 FirstVisibleChoice = 0;
	int32 HighlightedChoice = INDEX_NONE;
	TArray<FChopItDialogueChoiceView> CurrentChoices;
};
