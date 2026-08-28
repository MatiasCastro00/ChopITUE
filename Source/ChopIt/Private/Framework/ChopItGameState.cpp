#include "Framework/ChopItGameState.h"

#include "ChopItLogChannels.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Cycle/ChopItRunStateComponent.h"
#include "Cycle/ChopItWorldPresentationComponent.h"
#include "Economy/ChopItQuotaComponent.h"
#include "Economy/ChopItDayDefinition.h"
#include "Economy/ChopItEconomyComponent.h"
#include "Framework/ChopItPlayerState.h"
#include "Save/ChopItProfileSubsystem.h"
#include "Spawning/ChopItEnemyDirectorComponent.h"
#include "Spawning/ChopItEliteEncounterComponent.h"

AChopItGameState::AChopItGameState()
{
	PrimaryActorTick.bCanEverTick = false;
	QuotaComponent = CreateDefaultSubobject<UChopItQuotaComponent>(TEXT("QuotaComponent"));
	RunStateComponent = CreateDefaultSubobject<UChopItRunStateComponent>(TEXT("RunStateComponent"));
	CycleStateMachine = CreateDefaultSubobject<UChopItCycleStateMachineComponent>(TEXT("CycleStateMachine"));
	WorldPresentationComponent = CreateDefaultSubobject<UChopItWorldPresentationComponent>(TEXT("WorldPresentationComponent"));
	EnemyDirectorComponent = CreateDefaultSubobject<UChopItEnemyDirectorComponent>(TEXT("EnemyDirectorComponent"));
	EliteEncounterComponent = CreateDefaultSubobject<UChopItEliteEncounterComponent>(TEXT("EliteEncounterComponent"));
}

void AChopItGameState::BeginPlay()
{
	Super::BeginPlay();
	const UChopItDayDefinition* DayDefinition = LoadObject<UChopItDayDefinition>(
		nullptr,
		TEXT("/Game/ChopIt/Economy/Days/DA_Day_01.DA_Day_01"));
	const int32 InitialDay = ResolveInitialDay(DayDefinition);
	const int32 BaseQuota = DayDefinition ? DayDefinition->WoodQuota : 6;
	QuotaComponent->InitializeQuota(BaseQuota + FMath::Max(0, InitialDay - 1));
	RunStateComponent->InitializeRun(InitialDay);
	CycleStateMachine->ConfigureFromDay(DayDefinition);
	CycleStateMachine->StartCycle();
	RunStateComponent->OnResultChanged.AddUniqueDynamic(this,&AChopItGameState::HandleRunResult);
	UE_LOG(LogChopIt, Display, TEXT("Economy day initialized: quota=%d."), QuotaComponent->GetTarget());
}

int32 AChopItGameState::ResolveInitialDay(const UChopItDayDefinition* DayDefinition) const
{
	// Dedicated development map for exercising the post-day-seven decision
	// without weakening the normal run's progression.
	if (GetWorld() && GetWorld()->GetMapName().Contains(TEXT("L_Test_Infinite")))
	{
		return 7;
	}
	return DayDefinition ? DayDefinition->DayNumber : 1;
}

void AChopItGameState::HandleRunResult(EChopItRunResult Result,int32 DayNumber)
{
	if(Result!=EChopItRunResult::Defeat && Result!=EChopItRunResult::Victory) return;
	int64 Money=0; if(AChopItPlayerState* PS=GetWorld()->GetFirstPlayerController()?GetWorld()->GetFirstPlayerController()->GetPlayerState<AChopItPlayerState>():nullptr) if(UChopItEconomyComponent* E=PS->GetEconomyComponent()) Money=E->GetBalance();
	if(UChopItProfileSubsystem* Profile=GetGameInstance()->GetSubsystem<UChopItProfileSubsystem>()) Profile->CommitRun(Money,Result==EChopItRunResult::Victory);
}
