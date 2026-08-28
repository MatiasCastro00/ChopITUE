#pragma once

#include "Engine/DataAsset.h"
#include "ChopItEnemyDirectorDefinition.generated.h"

class AChopItEnemyCharacter;
class UChopItEnemyDefinition;

USTRUCT(BlueprintType)
struct CHOPITAI_API FChopItEnemySpawnEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftClassPtr<AChopItEnemyCharacter> EnemyClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UChopItEnemyDefinition> Definition;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1")) int32 BudgetCost = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0")) float Weight = 1.0f;
};

/** Data-driven night pacing knobs. It caps density independently of player power. */
UCLASS(BlueprintType)
class CHOPITAI_API UChopItEnemyDirectorDefinition final : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director", meta = (ClampMin = "0.1")) float WaveInterval = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director", meta = (ClampMin = "1")) int32 BaseBudget = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director", meta = (ClampMin = "0")) int32 BudgetGrowthPerWave = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director", meta = (ClampMin = "1")) int32 MaxAliveEnemies = 12;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director", meta = (ClampMin = "100.0")) float MinimumSpawnDistance = 700.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director", meta = (ClampMin = "100.0")) float MaximumSpawnDistance = 1450.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director", meta = (ClampMin = "100.0")) FVector2D SpawnBoundsExtent = FVector2D(1700.0f, 1700.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director") TArray<FChopItEnemySpawnEntry> NightEnemies;
};
