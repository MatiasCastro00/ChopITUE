#pragma once

#include "MVVMViewModelBase.h"
#include "ChopItPhonePresentationViewModel.generated.h"

class UMaterialInterface;

/** Observable presentation data; gameplay state remains on the phone. */
UCLASS(BlueprintType)
class CHOPIT_API UChopItPhonePresentationViewModel final : public UMVVMViewModelBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone") FText VariantName;
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone") FLinearColor AccentColor = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone") TObjectPtr<UMaterialInterface> PortraitMaterial;

	void SetVariantName(const FText& Value) { UE_MVVM_SET_PROPERTY_VALUE(VariantName, Value); }
	void SetAccentColor(const FLinearColor& Value) { UE_MVVM_SET_PROPERTY_VALUE(AccentColor, Value); }
	void SetPortraitMaterial(UMaterialInterface* Value) { UE_MVVM_SET_PROPERTY_VALUE(PortraitMaterial, Value); }
};
