#pragma once

#include "Engine/DataAsset.h"
#include "Events/ChopItSpecialEventPlacement.h"
#include "ChopItPhoneSpawnConfig.generated.h"

class AChopItMysteryPhone;

/** Designer controls for the first phone event. No terrain seed is stored in the asset. */
UCLASS(BlueprintType)
class CHOPITWORLD_API UChopItPhoneSpawnConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(ClampMin="0", ClampMax="1", ToolTip="One roll per run; zero disables spawning."))
	float SpawnProbability = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(ClampMin="0", ClampMax="16", ToolTip="Maximum phones attempted for a successful run roll."))
	int32 MaxPhones = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn")
	FChopItSpecialEventPlacementRules Placement;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(ToolTip="Optional Blueprint variant; native placeholder is used when empty."))
	TSubclassOf<AChopItMysteryPhone> PhoneClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(Units="cm", ToolTip="Vertical offset from the sampled terrain height."))
	float GroundOffset = 0.0f;

	bool IsValidConfig() const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
