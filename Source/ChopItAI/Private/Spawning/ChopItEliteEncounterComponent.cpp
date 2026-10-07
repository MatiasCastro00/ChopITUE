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
#include "TimerManager.h"

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
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(SpawnRetryTimer);
	if (ActiveElite.IsValid()) { ActiveElite->Destroy(); }
	Super::EndPlay(EndPlayReason);
}
void UChopItEliteEncounterComponent::HandlePhaseChanged(const EChopItCyclePhase NewPhase, const EChopItCyclePhase PreviousPhase, const int32 Generation)
{
	if (NewPhase == EChopItCyclePhase::Elite) { SpawnElite(); }
	else if (PreviousPhase == EChopItCyclePhase::Elite)
	{
		GetWorld()->GetTimerManager().ClearTimer(SpawnRetryTimer);
		if (ActiveElite.IsValid()) ActiveElite->Destroy();
		ActiveElite.Reset();
	}
}
void UChopItEliteEncounterComponent::SpawnElite()
{
	if (!GetWorld() || ActiveElite.IsValid()) return;
	const UChopItCycleStateMachineComponent* Cycle = GetOwner()->FindComponentByClass<UChopItCycleStateMachineComponent>();
	if (!Cycle || Cycle->GetCurrentPhase() != EChopItCyclePhase::Elite) return;
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	AActor* Player = PC ? PC->GetPawn() : nullptr;
	UChopItRunStateComponent* Run = GetOwner()->FindComponentByClass<UChopItRunStateComponent>();
	UChopItEnemyDefinition* Definition = Run && Run->GetDayNumber() >= 7 ? FinalDefinition.LoadSynchronous() : EliteDefinition.LoadSynchronous();
	if (!Player || !Definition)
	{
		UE_LOG(LogChopIt, Warning, TEXT("Elite spawn delayed: player=%s definition=%s."), *GetNameSafe(Player), *GetNameSafe(Definition));
		GetWorld()->GetTimerManager().SetTimer(SpawnRetryTimer, this, &ThisClass::SpawnElite, 1.0f, false);
		return;
	}
	const FVector Forward = Player->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	const FVector Origin = Player->GetActorLocation();
	const FVector Candidates[] = {Origin + Forward * 450.f, Origin + Right * 400.f,
		Origin - Right * 400.f, Origin - Forward * 350.f, Origin + Forward * 220.f};
	FVector SpawnLocation = Candidates[0];
	float GroundZ = Origin.Z;
	if (const UCapsuleComponent* PlayerCapsule = Player->FindComponentByClass<UCapsuleComponent>())
		GroundZ -= PlayerCapsule->GetScaledCapsuleHalfHeight();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ChopItEliteSpawnTrace), false, Player);
	for (const FVector& Candidate : Candidates)
	{
		FHitResult GroundHit;
		if (GetWorld()->LineTraceSingleByObjectType(GroundHit, Candidate + FVector::UpVector * 1200.f,
			Candidate - FVector::UpVector * 2000.f, FCollisionObjectQueryParams(ECC_WorldStatic), Params)
			&& GroundHit.ImpactNormal.Z > 0.55f)
		{
			SpawnLocation = Candidate;
			GroundZ = GroundHit.ImpactPoint.Z;
			break;
		}
	}
	const float HalfHeight = AChopItEnemyCharacter::StaticClass()->GetDefaultObject<AChopItEnemyCharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	SpawnLocation.Z = GroundZ + HalfHeight + 2.f;
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AChopItEnemyCharacter* Elite = GetWorld()->SpawnActor<AChopItEnemyCharacter>(AChopItEnemyCharacter::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (!Elite)
	{
		UE_LOG(LogChopIt, Warning, TEXT("Elite actor spawn failed at %s; retrying."), *SpawnLocation.ToCompactString());
		GetWorld()->GetTimerManager().SetTimer(SpawnRetryTimer, this, &ThisClass::SpawnElite, 1.0f, false);
		return;
	}
	Elite->InitializeFromDefinition(Definition, Player);
	Elite->GetHealthComponent()->OnDeath.AddUObject(this, &UChopItEliteEncounterComponent::HandleEliteDeath);
	ActiveElite = Elite;
	UE_LOG(LogChopIt, Display, TEXT("Elite encounter spawned: %s."), *Definition->DisplayName.ToString());
}
void UChopItEliteEncounterComponent::HandleEliteDeath(AActor* DeadActor, AActor* DamageSource)
{
	// Broadcast before the phase transition. Listeners can place rewards at the
	// still-valid elite location, including on the final victory transition.
	OnEliteDefeated.Broadcast(DeadActor, DamageSource);
	if (UChopItCycleStateMachineComponent* Cycle = GetOwner()->FindComponentByClass<UChopItCycleStateMachineComponent>())
	{
		Cycle->NotifyEliteDefeated(DeadActor);
	}
}
