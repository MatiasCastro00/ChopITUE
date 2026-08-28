#pragma once

#include "CoreMinimal.h"
#include "ChopItDamageTypes.generated.h"

USTRUCT(BlueprintType)
struct CHOPITCOMBAT_API FChopItDamageSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0.0"))
	float BaseDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	bool bCritical = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "1.0"))
	float CriticalMultiplier = 2.0f;

	float CalculateFinalDamage() const
	{
		const float CriticalScale = bCritical ? FMath::Max(1.0f, CriticalMultiplier) : 1.0f;
		return FMath::Max(0.0f, BaseDamage) * FMath::Max(0.0f, DamageMultiplier) * CriticalScale;
	}
};
