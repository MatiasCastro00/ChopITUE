#include "Spawning/ChopItEnemyDirectorComponent.h"

#include "ChopItLogChannels.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Cycle/ChopItRunStateComponent.h"
#include "Enemies/ChopItEnemyCharacter.h"
#include "Enemies/ChopItEnemyDefinition.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Spawning/ChopItEnemyDirectorDefinition.h"
#include "TimerManager.h"

UChopItEnemyDirectorComponent::UChopItEnemyDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	DirectorDefinition = TSoftObjectPtr<UChopItEnemyDirectorDefinition>(FSoftObjectPath(TEXT("/Game/ChopIt/AI/Directors/DA_Director_Day01.DA_Director_Day01")));
}

void UChopItEnemyDirectorComponent::BeginPlay()
{
	Super::BeginPlay();
	LoadedDefinition = DirectorDefinition.LoadSynchronous();
	if (UChopItCycleStateMachineComponent* Cycle = GetOwner()->FindComponentByClass<UChopItCycleStateMachineComponent>())
	{
		Cycle->OnPhaseChanged.AddUniqueDynamic(this, &UChopItEnemyDirectorComponent::HandlePhaseChanged);
		if (Cycle->GetCurrentPhase() == EChopItCyclePhase::Night) { StartNight(); }
	}
}

void UChopItEnemyDirectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopNight();
	Super::EndPlay(EndPlayReason);
}

int32 UChopItEnemyDirectorComponent::GetAliveEnemyCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AChopItEnemyCharacter>& Enemy : ActiveEnemies) { Count += Enemy.IsValid() ? 1 : 0; }
	return Count;
}

int32 UChopItEnemyDirectorComponent::CalculateWaveBudget(const int32 BaseBudget, const int32 GrowthPerWave, const int32 WaveIndex)
{
	return FMath::Max(0, BaseBudget) + FMath::Max(0, GrowthPerWave) * FMath::Max(0, WaveIndex);
}

void UChopItEnemyDirectorComponent::HandlePhaseChanged(const EChopItCyclePhase NewPhase, const EChopItCyclePhase PreviousPhase, const int32 Generation)
{
	if (NewPhase == EChopItCyclePhase::Night) { StartNight(); }
	else if (PreviousPhase == EChopItCyclePhase::Night) { StopNight(); }
}

void UChopItEnemyDirectorComponent::StartNight()
{
	if (!LoadedDefinition || !GetWorld()) { return; }
	WaveIndex = 0;
	SpawnWave();
	GetWorld()->GetTimerManager().SetTimer(WaveTimerHandle, this, &UChopItEnemyDirectorComponent::SpawnWave, LoadedDefinition->WaveInterval, true);
}

void UChopItEnemyDirectorComponent::StopNight()
{
	if (UWorld* World = GetWorld()) { World->GetTimerManager().ClearTimer(WaveTimerHandle); }
	for (const TWeakObjectPtr<AChopItEnemyCharacter>& Enemy : ActiveEnemies) { if (Enemy.IsValid()) { Enemy->Destroy(); } }
	ActiveEnemies.Reset();
}

void UChopItEnemyDirectorComponent::SpawnWave()
{
	PruneEnemies();
	if (!LoadedDefinition) { return; }
	const UChopItRunStateComponent* RunState = GetOwner()->FindComponentByClass<UChopItRunStateComponent>();
	const int32 DayNumber = RunState ? RunState->GetDayNumber() : 1;
	const bool bInfinite = RunState && RunState->IsInfiniteMode();
	const int32 InfiniteBonus = bInfinite ? 4 + FMath::Max(0, DayNumber - 8) * 2 : 0;
	const int32 MaxAlive = LoadedDefinition->MaxAliveEnemies + InfiniteBonus;
	if (GetAliveEnemyCount() >= MaxAlive) { return; }
	const int32 BaseBudget = LoadedDefinition->BaseBudget + FMath::Max(0, DayNumber - 1) + InfiniteBonus;
	const int32 Growth = LoadedDefinition->BudgetGrowthPerWave + (bInfinite ? 1 : 0);
	int32 Budget = CalculateWaveBudget(BaseBudget, Growth, WaveIndex++);
	while (Budget > 0 && GetAliveEnemyCount() < MaxAlive)
	{
		AChopItEnemyCharacter* Enemy = SpawnOne(Budget);
		if (!Enemy) { break; }
		const UChopItEnemyDefinition* Definition = Enemy->GetDefinition();
		Budget -= Definition && Definition->EnemyId == TEXT("FastTree") ? 2 : 1;
	}
}

void UChopItEnemyDirectorComponent::PruneEnemies()
{
	ActiveEnemies.RemoveAllSwap([](const TWeakObjectPtr<AChopItEnemyCharacter>& Enemy) { return !Enemy.IsValid(); });
}

AChopItEnemyCharacter* UChopItEnemyDirectorComponent::SpawnOne(const int32 AvailableBudget)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	AActor* Target = Controller ? Controller->GetPawn() : nullptr;
	if (!Target || !LoadedDefinition) { return nullptr; }
	TArray<const FChopItEnemySpawnEntry*> Candidates;
	float TotalWeight = 0.0f;
	for (const FChopItEnemySpawnEntry& Entry : LoadedDefinition->NightEnemies)
	{
		if (Entry.BudgetCost <= AvailableBudget && !Entry.EnemyClass.IsNull() && Entry.Weight > 0.0f) { Candidates.Add(&Entry); TotalWeight += Entry.Weight; }
	}
	if (Candidates.IsEmpty() || TotalWeight <= 0.0f) { return nullptr; }
	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	const FChopItEnemySpawnEntry* Picked = Candidates.Last();
	for (const FChopItEnemySpawnEntry* Candidate : Candidates) { Roll -= Candidate->Weight; if (Roll <= 0.0f) { Picked = Candidate; break; } }
	UClass* EnemyClass = Picked->EnemyClass.LoadSynchronous();
	if (!EnemyClass) { return nullptr; }
	FVector Candidate = FVector::ZeroVector;
	bool bInsideSpawnBounds = false;
	for (int32 Attempt = 0; Attempt < 8; ++Attempt)
	{
		const float Angle = FMath::FRandRange(0.0f, 360.0f);
		const float Distance = FMath::FRandRange(LoadedDefinition->MinimumSpawnDistance, LoadedDefinition->MaximumSpawnDistance);
		Candidate = Target->GetActorLocation() + FVector(FMath::Cos(FMath::DegreesToRadians(Angle)), FMath::Sin(FMath::DegreesToRadians(Angle)), 0.0f) * Distance;
		bInsideSpawnBounds = FMath::Abs(Candidate.X) <= LoadedDefinition->SpawnBoundsExtent.X
			&& FMath::Abs(Candidate.Y) <= LoadedDefinition->SpawnBoundsExtent.Y;
		if (bInsideSpawnBounds) { break; }
	}
	if (!bInsideSpawnBounds)
	{
		UE_LOG(LogChopIt, Verbose, TEXT("Enemy spawn deferred: no valid point inside bounds."));
		return nullptr;
	}
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ChopItEnemySpawnTrace), false, Target);
	const bool bFoundGround = GetWorld()->LineTraceSingleByObjectType(
		GroundHit,
		Candidate + FVector::UpVector * 5000.0f,
		Candidate - FVector::UpVector * 5000.0f,
		FCollisionObjectQueryParams(ECC_WorldStatic),
		QueryParams);
	if (!bFoundGround)
	{
		UE_LOG(LogChopIt, Warning, TEXT("Enemy spawn skipped: no ground under %s."), *Candidate.ToCompactString());
		return nullptr;
	}
	const float HalfHeight = AChopItEnemyCharacter::StaticClass()->GetDefaultObject<AChopItEnemyCharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Location(Candidate.X, Candidate.Y, GroundHit.ImpactPoint.Z + HalfHeight + 2.0f);
	AChopItEnemyCharacter* Enemy = GetWorld()->SpawnActor<AChopItEnemyCharacter>(EnemyClass, Location, FRotator::ZeroRotator);
	if (!Enemy) { return nullptr; }
	Enemy->InitializeFromDefinition(Picked->Definition.LoadSynchronous(), Target);
	ActiveEnemies.Add(Enemy);
	return Enemy;
}
