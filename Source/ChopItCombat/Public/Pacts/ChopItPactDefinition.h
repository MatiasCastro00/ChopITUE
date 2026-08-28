#pragma once
#include "Engine/DataAsset.h"
#include "Combat/ChopItCombatStatsComponent.h"
#include "ChopItPactDefinition.generated.h"
UCLASS(BlueprintType)
class CHOPITCOMBAT_API UChopItPactDefinition final : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName PactId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 CurseIncrease = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FChopItStatModifier> Modifiers;
};
