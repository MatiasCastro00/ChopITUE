#pragma once

#include "Blueprint/UserWidget.h"
#include "ChopItPhoneIndicatorWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
class UChopItPhoneIndicatorViewModel;

/** Passive comic view, driven only by field notifications from its view model. */
UCLASS(Blueprintable)
class CHOPIT_API UChopItPhoneIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetViewModel(UChopItPhoneIndicatorViewModel* InViewModel);
	void SetArtwork(UTexture2D* InBubbleTexture, UTexture2D* InRingTexture);
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone|UI")
	UChopItPhoneIndicatorViewModel* GetViewModel() const { return ViewModel; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	void BuildWidgetTree();
	void ApplyViewModel();
	void OnFieldChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId);

	UPROPERTY(Transient) TObjectPtr<UChopItPhoneIndicatorViewModel> ViewModel;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> BubbleTexture;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> RingTexture;
	UPROPERTY(Transient) TObjectPtr<UImage> BubbleImage;
	UPROPERTY(Transient) TObjectPtr<UImage> RingLeftImage;
	UPROPERTY(Transient) TObjectPtr<UImage> RingRightImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ArrowText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LabelText;
};
