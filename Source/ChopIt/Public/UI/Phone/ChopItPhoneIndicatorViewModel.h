#pragma once

#include "MVVMViewModelBase.h"
#include "ChopItPhoneIndicatorViewModel.generated.h"

/** UI state only; projection and call decisions stay outside the view model. */
UCLASS(BlueprintType)
class CHOPIT_API UChopItPhoneIndicatorViewModel final : public UMVVMViewModelBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone")
	FText Label;
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone")
	FVector2D ScreenPosition = FVector2D::ZeroVector;
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone")
	float Scale = 1.0f;
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone")
	float Opacity = 0.0f;
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone")
	float ArrowDegrees = 0.0f;
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone")
	bool bVisible = false;
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone")
	bool bOffscreen = false;
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone")
	float RingPulse = 0.0f;
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Phone")
	float TiltDegrees = 0.0f;

	void SetLabel(const FText& Value) { UE_MVVM_SET_PROPERTY_VALUE(Label, Value); }
	void SetScreenPosition(const FVector2D Value) { UE_MVVM_SET_PROPERTY_VALUE(ScreenPosition, Value); }
	void SetScale(const float Value) { UE_MVVM_SET_PROPERTY_VALUE(Scale, Value); }
	void SetOpacity(const float Value) { UE_MVVM_SET_PROPERTY_VALUE(Opacity, Value); }
	void SetArrowDegrees(const float Value) { UE_MVVM_SET_PROPERTY_VALUE(ArrowDegrees, Value); }
	void SetVisible(const bool Value) { UE_MVVM_SET_PROPERTY_VALUE(bVisible, Value); }
	void SetOffscreen(const bool Value) { UE_MVVM_SET_PROPERTY_VALUE(bOffscreen, Value); }
	void SetRingPulse(const float Value) { UE_MVVM_SET_PROPERTY_VALUE(RingPulse, Value); }
	void SetTiltDegrees(const float Value) { UE_MVVM_SET_PROPERTY_VALUE(TiltDegrees, Value); }
};
