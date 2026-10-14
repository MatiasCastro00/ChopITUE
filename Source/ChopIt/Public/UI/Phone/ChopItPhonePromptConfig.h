#pragma once

#include "Engine/DataAsset.h"
#include "ChopItPhonePromptConfig.generated.h"

class UChopItPhonePromptWidget;

/** Presentation settings for the phone's nearby interaction key. */
UCLASS(BlueprintType)
class CHOPIT_API UChopItPhonePromptConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FText ActionText;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Content")
	FText FallbackKeyText = FText::FromString(TEXT("E"));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Content")
	TSubclassOf<UChopItPhonePromptWidget> WidgetClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Placement", meta=(Units="cm"))
	float HeightAbovePhone = 130.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Placement")
	FVector2D DrawSize = FVector2D(112, 120);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Detection", meta=(ClampMin="0", Units="cm"))
	float DetectionPadding = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Detection", meta=(ClampMin="0.02", Units="s"))
	float CheckPeriod = 0.07f;

	bool IsValidConfig() const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
