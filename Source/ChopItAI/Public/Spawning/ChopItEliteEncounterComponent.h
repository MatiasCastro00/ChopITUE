#pragma once

#include "Components/ActorComponent.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "ChopItEliteEncounterComponent.generated.h"

class AChopItEnemyCharacter;
class UChopItEnemyDefinition;

/** Spawns one cycle-ending elite and unlocks the cycle only after its death. */
UCLASS(ClassGroup = (ChopIt), meta = (BlueprintSpawnableComponent))
class CHOPITAI_API UChopItEliteEncounterComponent final : public UActorComponent
{
	GENERATED_BODY()
public:
	UChopItEliteEncounterComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	UFUNCTION() void HandlePhaseChanged(EChopItCyclePhase NewPhase, EChopItCyclePhase PreviousPhase, int32 Generation);
	void SpawnElite();
	void HandleEliteDeath(AActor* DeadActor, AActor* DamageSource);
	TWeakObjectPtr<AChopItEnemyCharacter> ActiveElite;
	UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<UChopItEnemyDefinition> EliteDefinition;
	UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<UChopItEnemyDefinition> FinalDefinition;
};
