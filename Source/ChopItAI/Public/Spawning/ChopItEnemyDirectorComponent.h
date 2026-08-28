#pragma once

#include "Components/ActorComponent.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "ChopItEnemyDirectorComponent.generated.h"

class AChopItEnemyCharacter;
class UChopItEnemyDirectorDefinition;

/** Owns active night enemies and spends a bounded design budget at timed waves. */
UCLASS(ClassGroup = (ChopIt), BlueprintType, meta = (BlueprintSpawnableComponent))
class CHOPITAI_API UChopItEnemyDirectorComponent final : public UActorComponent
{
	GENERATED_BODY()
public:
	UChopItEnemyDirectorComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	int32 GetAliveEnemyCount() const;
	static int32 CalculateWaveBudget(int32 BaseBudget, int32 GrowthPerWave, int32 WaveIndex);
private:
	UFUNCTION() void HandlePhaseChanged(EChopItCyclePhase NewPhase, EChopItCyclePhase PreviousPhase, int32 Generation);
	void StartNight();
	void StopNight();
	void SpawnWave();
	void PruneEnemies();
	AChopItEnemyCharacter* SpawnOne(int32 AvailableBudget);
	UPROPERTY(EditDefaultsOnly, Category = "ChopIt|Director") TSoftObjectPtr<UChopItEnemyDirectorDefinition> DirectorDefinition;
	TObjectPtr<UChopItEnemyDirectorDefinition> LoadedDefinition;
	TArray<TWeakObjectPtr<AChopItEnemyCharacter>> ActiveEnemies;
	FTimerHandle WaveTimerHandle;
	int32 WaveIndex = 0;
};
