#pragma once

#include "Engine/DataAsset.h"
#include "ChopItPhoneCallConfig.generated.h"

class USoundBase;
class USoundAttenuation;

/** Timing and 3D falloff for the mystery phone's recurring call. */
UCLASS(BlueprintType)
class CHOPITWORLD_API UChopItPhoneCallConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Call")
	TObjectPtr<USoundBase> RingSound;
	/** Optional Sound Attenuation asset for a designer-authored distance curve. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
	TObjectPtr<USoundAttenuation> AttenuationAsset;
	/** Time between the starts of two rings, including the ring duration. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Call", meta=(ClampMin="0.1", Units="s"))
	float MinIntervalSeconds = 10.0f;
	/** Delay before the first ring; later rings use the random interval below. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Call", meta=(ClampMin="0.1", Units="s"))
	float FirstCallDelaySeconds = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Call", meta=(ClampMin="0.1", Units="s"))
	float MaxIntervalSeconds = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Call", meta=(ClampMin="0.1", Units="s"))
	float RingDurationSeconds = 3.0f;
	/** Start-to-start spacing for the three double-ring groups. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Call", meta=(ClampMin="0.1", Units="s"))
	float RingPairSpacingSeconds = 1.0f;
	/** Space between the two strikes in each group. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Call", meta=(ClampMin="0.05", Units="s"))
	float RingStrikeSpacingSeconds = 0.32f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Call", meta=(ClampMin="1"))
	int32 RingPairsPerCall = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0", Units="cm"))
	float FullVolumeRadius = 110.0f;
	/** Compatibility fallback for owners that are not mystery phones. BP_MysteryPhone owns the editable radius. */
	UPROPERTY()
	float AudibleDistance = 850.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ClampMin="0.0", ClampMax="4.0"))
	float Volume = 0.8f;

	bool IsValidConfig() const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
