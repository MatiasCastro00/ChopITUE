#pragma once

#include "Engine/DataAsset.h"
#include "ChopItEnemyDefinition.generated.h"

/** Static tuning for an enemy family. The director owns when it may spawn. */
UCLASS(BlueprintType)
class CHOPITAI_API UChopItEnemyDefinition final : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy") FName EnemyId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "1.0")) float MaxHealth = 45.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0.0")) float MoveSpeed = 250.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Attack", meta = (ClampMin = "0.0")) float ContactDamage = 8.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Attack", meta = (ClampMin = "0.05")) float AttackInterval = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Attack", meta = (ClampMin = "1.0")) float AttackRange = 115.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Rewards", meta = (ClampMin = "0")) int32 ExperienceReward = 6;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Rewards", meta = (ClampMin = "1")) int32 WoodRewardUnits = 1;
};
