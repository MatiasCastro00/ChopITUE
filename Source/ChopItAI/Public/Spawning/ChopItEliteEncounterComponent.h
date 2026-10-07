#pragma once

#include "Components/ActorComponent.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "ChopItEliteEncounterComponent.generated.h"

class AChopItEnemyCharacter;
class UChopItEnemyDefinition;

DECLARE_MULTICAST_DELEGATE_TwoParams(FChopItEliteDefeatedNative, AActor* /*Elite*/, AActor* /*Killer*/);

/** Spawns one cycle-ending elite and unlocks the cycle only after its death. */
UCLASS(ClassGroup = (ChopIt), meta = (BlueprintSpawnableComponent))
class CHOPITAI_API UChopItEliteEncounterComponent final : public UActorComponent
{
	GENERATED_BODY()
public:
	UChopItEliteEncounterComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	FChopItEliteDefeatedNative OnEliteDefeated;
	AChopItEnemyCharacter* GetActiveElite() const { return ActiveElite.Get(); }
private:
	UFUNCTION() void HandlePhaseChanged(EChopItCyclePhase NewPhase, EChopItCyclePhase PreviousPhase, int32 Generation);
	void SpawnElite();
	void HandleEliteDeath(AActor* DeadActor, AActor* DamageSource);
	TWeakObjectPtr<AChopItEnemyCharacter> ActiveElite;
	FTimerHandle SpawnRetryTimer;
	UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<UChopItEnemyDefinition> EliteDefinition;
	UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<UChopItEnemyDefinition> FinalDefinition;
};
