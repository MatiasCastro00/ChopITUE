#pragma once

#include "Engine/DataAsset.h"
#include "ChopItPhoneIndicatorConfig.generated.h"

class UCurveFloat;
class UTexture2D;
class UChopItPhoneIndicatorWidget;

/** Designer controls for the reusable RING RING screen indicator. */
UCLASS(BlueprintType)
class CHOPIT_API UChopItPhoneIndicatorConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Content")
	FText Label = FText::FromString(TEXT("RING RING!"));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Content")
	TSubclassOf<UChopItPhoneIndicatorWidget> WidgetClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Artwork")
	TObjectPtr<UTexture2D> BubbleTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Artwork")
	TObjectPtr<UTexture2D> RingTexture;
	/** Fixed width of the complete UMG frame in the world, independent of hearing range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Layout", meta=(ClampMin="1", Units="cm"))
	float WorldWidth = 140.0f;
	/** Fixed height of the comic's center above the phone actor's origin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Layout", meta=(ClampMin="0", Units="cm"))
	float WorldHeight = 95.0f;
	/** Move the comic toward the camera's right, while keeping its world-space perspective. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Layout", meta=(Units="cm"))
	float WorldRightOffset = 45.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Layout", meta=(ClampMin="0"))
	float ScreenMargin = 105.0f;
	// Legacy screen-space settings, no longer used by world projection.
	UPROPERTY()
	FVector2D OnscreenOffset = FVector2D::ZeroVector;
	UPROPERTY()
	float MinScale = 0.38f;
	UPROPERTY()
	float MaxScale = 1.15f;
	// Retained only for loading existing assets and migration scripts.
	UPROPERTY()
	float DistanceScaleExponent = 1.35f;
	UPROPERTY()
	float OnscreenScaleMultiplier = 0.95f;
	UPROPERTY()
	float OffscreenScaleMultiplier = 1.05f;
	UPROPERTY()
	TObjectPtr<UCurveFloat> DistanceScaleCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation", meta=(ClampMin="0.01", Units="s"))
	float UpdatePeriod = 0.033f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation", meta=(ClampMin="0.01", Units="s"))
	float EntranceDuration = 0.28f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation", meta=(ClampMin="0.01", Units="s"))
	float ExitDuration = 0.22f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation", meta=(ClampMin="0.0", Units="s"))
	float TypeDuration = 0.24f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation", meta=(ClampMin="0.0"))
	float BounceOvershoot = 0.18f;
	UPROPERTY()
	float PositionSmoothness = 12.0f;

	bool IsValidConfig() const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
