#pragma once

#include "Blueprint/UserWidget.h"
#include "MVVMViewModelBase.h"
#include "ChopItPhonePresentationWidget.generated.h"

class UChopItPhonePresentationViewModel;
class UBorder;
class UImage;
class UTextBlock;

/** Phone portrait layer behind the reusable dialogue bubble. */
UCLASS(Blueprintable)
class CHOPIT_API UChopItPhonePresentationWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetViewModel(UChopItPhonePresentationViewModel* InViewModel);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
private:
	void ApplyViewModel();
	void OnFieldChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId);
	UPROPERTY(Transient) TObjectPtr<UChopItPhonePresentationViewModel> ViewModel;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UImage> Portrait;
};
