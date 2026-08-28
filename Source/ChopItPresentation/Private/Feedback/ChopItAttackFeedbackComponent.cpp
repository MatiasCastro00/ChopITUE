#include "Feedback/ChopItAttackFeedbackComponent.h"

#include "ChopItDeveloperSettings.h"
#include "Feedback/ChopItAxeSwingTrail.h"
#include "Feedback/ChopItFeedbackAudio.h"
#include "Weapons/ChopItAutoAttackComponent.h"

UChopItAttackFeedbackComponent::UChopItAttackFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItAttackFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UChopItAutoAttackComponent* AutoAttack = GetOwner()->FindComponentByClass<UChopItAutoAttackComponent>())
	{
		AutoAttack->OnAttackPerformed.AddUObject(this, &UChopItAttackFeedbackComponent::HandleAttackPerformed);
	}
}

void UChopItAttackFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UChopItAutoAttackComponent* AutoAttack = GetOwner()->FindComponentByClass<UChopItAutoAttackComponent>())
	{
		AutoAttack->OnAttackPerformed.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void UChopItAttackFeedbackComponent::HandleAttackPerformed(const FVector& Origin, const FVector& Forward, const float Range, const bool bHit)
{
	const UChopItDeveloperSettings* Settings = GetDefault<UChopItDeveloperSettings>();
	UWorld* World = GetWorld();
	if (!Settings || !World || Settings->EffectsDensity <= 0.0f || FMath::FRand() > Settings->EffectsDensity)
	{
		return;
	}
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AChopItAxeSwingTrail* Trail = World->SpawnActor<AChopItAxeSwingTrail>(AChopItAxeSwingTrail::StaticClass(), Origin, FRotator::ZeroRotator, SpawnParameters))
	{
		Trail->InitializeTrail(Forward, Range, bHit);
	}
	if (bHit && Settings->bEnableImpactSounds)
	{
		ChopItFeedbackAudio::PlayAxeSwing(this, Origin, Settings->EffectsVolume);
	}
}
