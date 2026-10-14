#pragma once

#include "Engine/DataAsset.h"
#include "ChopItPhoneVariantPool.generated.h"

class UChopItPhoneVariantData;

/** Extensible weighted selection; adding a variant only changes this asset. */
UCLASS(BlueprintType)
class CHOPITWORLD_API UChopItPhoneVariantPool : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection")
	TArray<TObjectPtr<UChopItPhoneVariantData>> Variants;

	bool IsValidConfig() const;
	float GetTotalWeight() const;
	const UChopItPhoneVariantData* SelectByRoll(float Roll) const;
	const UChopItPhoneVariantData* Select(FRandomStream& Random) const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
