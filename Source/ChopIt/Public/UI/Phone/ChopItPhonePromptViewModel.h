#pragma once

#include "MVVMViewModelBase.h"
#include "ChopItPhonePromptViewModel.generated.h"

/** Passive prompt data observed by the world-space UMG view. */
UCLASS(BlueprintType)
class CHOPIT_API UChopItPhonePromptViewModel final : public UMVVMViewModelBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone")
	FText KeyText;
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone")
	FText ActionText;
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone")
	bool bVisible = false;

	void SetKeyText(const FText& Value) { UE_MVVM_SET_PROPERTY_VALUE(KeyText, Value); }
	void SetActionText(const FText& Value) { UE_MVVM_SET_PROPERTY_VALUE(ActionText, Value); }
	void SetVisible(const bool Value) { UE_MVVM_SET_PROPERTY_VALUE(bVisible, Value); }
};
