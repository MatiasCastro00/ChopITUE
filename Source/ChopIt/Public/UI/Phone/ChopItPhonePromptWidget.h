#pragma once

#include "Blueprint/UserWidget.h"
#include "MVVMViewModelBase.h"
#include "ChopItPhonePromptWidget.generated.h"

class UCanvasPanel;
class UImage;
class UChopItPhonePromptViewModel;

/** WidgetComponent view; reads only the prompt ViewModel. */
UCLASS(Blueprintable)
class CHOPIT_API UChopItPhonePromptWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetViewModel(UChopItPhonePromptViewModel* InViewModel);
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone|UI")
	UChopItPhonePromptViewModel* GetViewModel() const { return ViewModel; }
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation", meta=(ClampMin="0"))
	float BobHeight = 5.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation", meta=(ClampMin="0"))
	float BobFrequency = 1.8f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

private:
	void ApplyViewModel();
	void OnFieldChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId);

	UPROPERTY(Transient) TObjectPtr<UChopItPhonePromptViewModel> ViewModel;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UCanvasPanel> KeyGroup;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UImage> KeyPlate;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UImage> KeyGlyph;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UImage> RayLeft;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UImage> RayCenter;
	UPROPERTY(Transient, meta=(BindWidget)) TObjectPtr<UImage> RayRight;
	float AnimationTime = 0.0f;
	bool bWasVisible = false;
};
