#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Dialogue/ChopItDialogueAssets.h"
#include "Dialogue/ChopItDialogueMarkup.h"
#include "Dialogue/ChopItDialogueTypes.h"
#include "ChopItDialogueWidget.generated.h"

class UBorder;
class UCanvasPanelSlot;
class UChopItAnimatedTextBlock;
class UHorizontalBox;
class UImage;
class UOverlay;
class USafeZone;
class USizeBox;
class UTextBlock;
class UVerticalBox;

/** Passive, native dialogue view. Runtime rules remain in UChopItDialogueSubsystem. */
UCLASS()
class CHOPITPRESENTATION_API UChopItDialogueWidget final : public UUserWidget
{
	GENERATED_BODY()
public:
	void ConfigureTheme(const UChopItDialogueTheme* InTheme, bool bInReduceMotion, float InUIScale = 1.0f);
	void ShowLine(const FChopItDialogueLine& Line, const FChopItDialogueMarkupDocument& Document);
	void SetRevealCount(int32 Count);
	void SetChoices(const TArray<FChopItDialogueChoiceView>& Choices, int32 SelectedIndex);
	void SetSelectedChoice(int32 SelectedIndex);
	void SetPortraitExpression(const UChopItDialogueSpeakerDefinition* Speaker, FName Expression, bool bReact = true);
	void SetAwaitingAdvance(bool bAwaiting);
	void SetTransitionProgress(float Alpha, bool bExiting, bool bWholePanel);
	void ResetTransition();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildWidgetTree();
	void ArrangePortrait(EChopItDialoguePortraitSide Side);
	void ApplyChoiceStyle(int32 Index);
	void UpdatePanelForChoiceCount(int32 ChoiceCount);

	UPROPERTY(Transient) TObjectPtr<USafeZone> SafeZone;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> SafeSlot;
	UPROPERTY(Transient) TObjectPtr<UBorder> OuterBorder;
	UPROPERTY(Transient) TObjectPtr<UBorder> InnerBorder;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> DialogueRow;
	UPROPERTY(Transient) TObjectPtr<USizeBox> PortraitBox;
	UPROPERTY(Transient) TObjectPtr<UBorder> PortraitFrame;
	UPROPERTY(Transient) TObjectPtr<UBorder> PortraitBackdrop;
	UPROPERTY(Transient) TObjectPtr<UOverlay> PortraitOverlay;
	UPROPERTY(Transient) TObjectPtr<UImage> PreviousPortrait;
	UPROPERTY(Transient) TObjectPtr<UImage> CurrentPortrait;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> TextArea;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SpeakerName;
	UPROPERTY(Transient) TObjectPtr<UChopItAnimatedTextBlock> AnimatedText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> ChoiceBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> AdvanceIndicator;
	UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> ChoiceBorders;

	TWeakObjectPtr<const UChopItDialogueTheme> Theme;
	bool bReduceMotion = false;
	float PortraitBlendElapsed = 1.0f;
	float IndicatorTime = 0.0f;
	float CachedUIScale = 1.0f;
	FLinearColor CachedChoiceColor = FLinearColor(0.18f, 0.11f, 0.055f, 0.98f);
	FLinearColor CachedSelectedColor = FLinearColor(0.85f, 0.22f, 0.025f, 1.0f);
};
