#include "Weapons/ChopItAutoAttackComponent.h"

#include "Combat/ChopItCombatStatsComponent.h"
#include "Combat/ChopItDamageTypes.h"
#include "Combat/ChopItHealthComponent.h"
#include "DrawDebugHelpers.h"
#include "Targeting/ChopItTargetingSubsystem.h"
#include "Weapons/ChopItWeaponDefinition.h"

namespace
{
	void DrawAttackArc(UWorld* World, const FVector& Origin, const FVector& Forward, const float Range, const float HalfAngle)
	{
		constexpr int32 SegmentCount = 14;
		const FVector FlatForward = FVector(Forward.X, Forward.Y, 0.0f).GetSafeNormal();
		const FVector RaisedOrigin = Origin + FVector(0.0f, 0.0f, 55.0f);
		FVector PreviousPoint = RaisedOrigin + FlatForward.RotateAngleAxis(-HalfAngle, FVector::UpVector) * Range;
		DrawDebugLine(World, RaisedOrigin, PreviousPoint, FColor::Orange, false, 0.35f, 0, 9.0f);
		for (int32 Segment = 1; Segment <= SegmentCount; ++Segment)
		{
			const float Alpha = static_cast<float>(Segment) / static_cast<float>(SegmentCount);
			const float Angle = FMath::Lerp(-HalfAngle, HalfAngle, Alpha);
			const FVector Point = RaisedOrigin + FlatForward.RotateAngleAxis(Angle, FVector::UpVector) * Range;
			DrawDebugLine(World, PreviousPoint, Point, FColor::Orange, false, 0.35f, 0, 9.0f);
			PreviousPoint = Point;
		}
		DrawDebugLine(World, RaisedOrigin, PreviousPoint, FColor::Orange, false, 0.35f, 0, 9.0f);
	}
}

UChopItAutoAttackComponent::UChopItAutoAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItAutoAttackComponent::BeginPlay()
{
	Super::BeginPlay();
	StatsComponent = GetOwner()->FindComponentByClass<UChopItCombatStatsComponent>();
	if (StatsComponent)
	{
		StatsComponent->OnStatsChanged.AddUObject(this, &UChopItAutoAttackComponent::HandleStatsChanged);
	}
	if (!WeaponDefinition)
	{
		WeaponDefinition = LoadObject<UChopItWeaponDefinition>(
			nullptr,
			TEXT("/Game/ChopIt/Combat/Weapons/DA_Weapon_BasicAxe.DA_Weapon_BasicAxe"));
	}
	RestartAttackTimer();
}

void UChopItAutoAttackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackTimerHandle);
	}
	if (StatsComponent)
	{
		StatsComponent->OnStatsChanged.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void UChopItAutoAttackComponent::HandleStatsChanged()
{
	RestartAttackTimer();
}

void UChopItAutoAttackComponent::SetWeaponDefinition(UChopItWeaponDefinition* NewDefinition)
{
	WeaponDefinition = NewDefinition;
	if (HasBegunPlay())
	{
		RestartAttackTimer();
	}
}

void UChopItAutoAttackComponent::RestartAttackTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	World->GetTimerManager().ClearTimer(AttackTimerHandle);
	if (!WeaponDefinition)
	{
		return;
	}

	const float BaseAttackSpeed = 1.0f;
	const float AttackSpeed = StatsComponent
		? StatsComponent->EvaluateStat(EChopItCombatStat::AttackSpeed, BaseAttackSpeed)
		: BaseAttackSpeed;
	const float Interval = WeaponDefinition->AttackInterval / FMath::Max(0.05f, AttackSpeed);
	World->GetTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&UChopItAutoAttackComponent::PerformAttack,
		Interval,
		true,
		FMath::Min(0.2f, Interval));
}

void UChopItAutoAttackComponent::PerformAttack()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || !WeaponDefinition)
	{
		return;
	}

	const float Range = StatsComponent
		? StatsComponent->EvaluateStat(EChopItCombatStat::Range, WeaponDefinition->Range)
		: WeaponDefinition->Range;
	const FVector AttackForward = Owner->GetActorForwardVector();
	if (WeaponDefinition->AttackPattern == EChopItWeaponAttackPattern::ArcMelee)
	{
		DrawAttackArc(World, Owner->GetActorLocation(), AttackForward, Range, WeaponDefinition->ArcHalfAngleDegrees);
	}
	else
	{
		DrawDebugCircle(World, Owner->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f), Range, 20, FColor::Cyan, false, 0.25f, 0, 7.0f, FVector::ForwardVector, FVector::RightVector, false);
	}

	UChopItTargetingSubsystem* Targeting = World->GetSubsystem<UChopItTargetingSubsystem>();
	const TArray<UChopItHealthComponent*> Targets = Targeting
		? (WeaponDefinition->AttackPattern == EChopItWeaponAttackPattern::RadialMelee
			? Targeting->FindTargetsInRadius(Owner->GetActorLocation(), Range, WeaponDefinition->MaxTargets, Owner)
			: Targeting->FindTargetsInArc(Owner->GetActorLocation(), AttackForward, Range, WeaponDefinition->ArcHalfAngleDegrees, WeaponDefinition->MaxTargets, Owner))
		: TArray<UChopItHealthComponent*>();

	const float Damage = StatsComponent
		? StatsComponent->EvaluateStat(EChopItCombatStat::Damage, WeaponDefinition->Damage)
		: WeaponDefinition->Damage;
	const float CriticalChance = StatsComponent
		? StatsComponent->EvaluateStat(EChopItCombatStat::CriticalChance, WeaponDefinition->CriticalChance)
		: WeaponDefinition->CriticalChance;

	bool bHitAtLeastOneTarget = false;
	for (UChopItHealthComponent* Target : Targets)
	{
		if (!IsValid(Target))
		{
			continue;
		}
		FChopItDamageSpec DamageSpec;
		DamageSpec.BaseDamage = Damage;
		DamageSpec.bCritical = FMath::FRand() < FMath::Clamp(CriticalChance, 0.0f, 1.0f);
		DamageSpec.CriticalMultiplier = WeaponDefinition->CriticalMultiplier;
		AActor* TargetActor = Target->GetOwner();
		FVector ImpactLocation = Owner->GetActorLocation() + AttackForward * Range;
		if (TargetActor)
		{
			FVector ToTarget = TargetActor->GetActorLocation() - Owner->GetActorLocation();
			ToTarget.Z = 0.0f;
			const FVector HitSide = ToTarget.GetSafeNormal2D(UE_SMALL_NUMBER, AttackForward);
			ImpactLocation = TargetActor->GetActorLocation() - HitSide * 52.0f + FVector(0.0f, 0.0f, 65.0f);
		}
		bHitAtLeastOneTarget |= Target->ApplyDamage(DamageSpec, Owner, ImpactLocation) > 0.0f;
	}
	OnAttackPerformed.Broadcast(Owner->GetActorLocation(), AttackForward, Range, bHitAtLeastOneTarget);
}
