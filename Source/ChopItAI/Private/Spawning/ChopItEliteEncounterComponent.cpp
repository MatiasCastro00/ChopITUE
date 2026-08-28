#include "Spawning/ChopItEliteEncounterComponent.h"

#include "ChopItLogChannels.h"
#include "Combat/ChopItHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Cycle/ChopItRunStateComponent.h"
#include "Enemies/ChopItEnemyCharacter.h"
#include "Enemies/ChopItEnemyDefinition.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UChopItEliteEncounterComponent::UChopItEliteEncounterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	EliteDefinition = TSoftObjectPtr<UChopItEnemyDefinition>(FSoftObjectPath(TEXT("/Game/ChopIt/AI/Enemies/DA_Enemy_Guardian.DA_Enemy_Guardian")));
	FinalDefinition = TSoftObjectPtr<UChopItEnemyDefinition>(FSoftObjectPath(TEXT("/Game/ChopIt/AI/Enemies/DA_Enemy_ForestEntity.DA_Enemy_ForestEntity")));
}
void UChopItEliteEncounterComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UChopItCycleStateMachineComponent* Cycle = GetOwner()->FindComponentByClass<UChopItCycleStateMachineComponent>())
	{
		Cycle->OnPhaseChanged.AddUniqueDynamic(this, &UChopItEliteEncounterComponent::HandlePhaseChanged);
	}
}
void UChopItEliteEncounterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ActiveElite.IsValid()) { ActiveElite->Destroy(); }
	Super::EndPlay(EndPlayReason);
}
void UChopItEliteEncounterComponent::HandlePhaseChanged(const EChopItCyclePhase NewPhase, const EChopItCyclePhase PreviousPhase, const int32 Generation)
{
	if (NewPhase == EChopItCyclePhase::Elite) { SpawnElite(); }
	else if (PreviousPhase == EChopItCyclePhase::Elite && ActiveElite.IsValid()) { ActiveElite->Destroy(); ActiveElite.Reset(); }
}
void UChopItEliteEncounterComponent::SpawnElite()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	AActor* Player = PC ? PC->GetPawn() : nullptr;
	UChopItRunStateComponent* Run = GetOwner()->FindComponentByClass<UChopItRunStateComponent>();
	UChopItEnemyDefinition* Definition = Run && Run->GetDayNumber() >= 7 ? FinalDefinition.LoadSynchronous() : EliteDefinition.LoadSynchronous();
	if (!Player || !Definition) { return; }
	const FVector Candidate = Player->GetActorLocation() + FVector(850.0f, 0.0f, 0.0f);
	FHitResult GroundHit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ChopItEliteSpawnTrace), false, Player);
	if (!GetWorld()->LineTraceSingleByObjectType(GroundHit, Candidate + FVector::UpVector * 5000.0f, Candidate - FVector::UpVector * 5000.0f, FCollisionObjectQueryParams(ECC_WorldStatic), Params)) { return; }
	const float HalfHeight = AChopItEnemyCharacter::StaticClass()->GetDefaultObject<AChopItEnemyCharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	AChopItEnemyCharacter* Elite = GetWorld()->SpawnActor<AChopItEnemyCharacter>(AChopItEnemyCharacter::StaticClass(), FVector(Candidate.X, Candidate.Y, GroundHit.ImpactPoint.Z + HalfHeight + 2.0f), FRotator::ZeroRotator);
	if (!Elite) { return; }
	Elite->InitializeFromDefinition(Definition, Player);
	Elite->GetHealthComponent()->OnDeath.AddUObject(this, &UChopItEliteEncounterComponent::HandleEliteDeath);
	ActiveElite = Elite;
	UE_LOG(LogChopIt, Display, TEXT("Elite encounter spawned: %s."), *Definition->DisplayName.ToString());
}
void UChopItEliteEncounterComponent::HandleEliteDeath(AActor* DeadActor, AActor* DamageSource)
{
	if (UChopItCycleStateMachineComponent* Cycle = GetOwner()->FindComponentByClass<UChopItCycleStateMachineComponent>())
	{
		Cycle->NotifyEliteDefeated(DeadActor);
	}
}
