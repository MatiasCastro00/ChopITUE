#include "Feedback/ChopItHitFeedbackComponent.h"

#include "ChopItDeveloperSettings.h"
#include "Combat/ChopItHealthComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Feedback/ChopItFeedbackAudio.h"
#include "Feedback/ChopItFeedbackBurst.h"
#include "Feedback/ChopItDamageNumber.h"
#include "Feedback/ChopItLeafFall.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"

UChopItHitFeedbackComponent::UChopItHitFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChopItHitFeedbackComponent::SetVisualComponent(UPrimitiveComponent* NewVisual)
{
	VisualComponent = NewVisual;
}

void UChopItHitFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!VisualComponent.IsValid())
	{
		VisualComponent = GetOwner()->FindComponentByClass<UPrimitiveComponent>();
	}
	if (VisualComponent.IsValid())
	{
		OriginalVisualScale = VisualComponent->GetRelativeScale3D();
	}
	CameraBoom = GetOwner()->FindComponentByClass<USpringArmComponent>();
	if (CameraBoom.IsValid())
	{
		OriginalCameraOffset = CameraBoom->TargetOffset;
	}
	if (UChopItHealthComponent* Health = GetOwner()->FindComponentByClass<UChopItHealthComponent>())
	{
		Health->OnDamageReceived.AddUObject(this, &UChopItHitFeedbackComponent::HandleDamageReceived);
		Health->OnDeath.AddUObject(this, &UChopItHitFeedbackComponent::HandleDeath);
	}
}

void UChopItHitFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PulseTimerHandle);
		World->GetTimerManager().ClearTimer(CameraTimerHandle);
	}
	if (UChopItHealthComponent* Health = GetOwner()->FindComponentByClass<UChopItHealthComponent>())
	{
		Health->OnDamageReceived.RemoveAll(this);
		Health->OnDeath.RemoveAll(this);
	}
	RestorePulse();
	RestoreCamera();
	Super::EndPlay(EndPlayReason);
}

void UChopItHitFeedbackComponent::HandleDamageReceived(const float Damage, const bool bCritical, AActor* DamageSource, const FVector& ImpactLocation)
{
	const UChopItDeveloperSettings* Settings = GetDefault<UChopItDeveloperSettings>();
	UWorld* World = GetWorld();
	if (!Settings || !World)
	{
		return;
	}
	if (Settings->bEnableHitFlash && VisualComponent.IsValid())
	{
		const float Scale = bCritical ? 1.24f : 1.10f;
		VisualComponent->SetRelativeScale3D(OriginalVisualScale * Scale);
		World->GetTimerManager().SetTimer(PulseTimerHandle, this, &UChopItHitFeedbackComponent::RestorePulse, PulseDuration, false);
	}
	if (Settings->bEnableDamageNumbers)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AChopItDamageNumber* DamageNumber = World->SpawnActor<AChopItDamageNumber>(AChopItDamageNumber::StaticClass(), ImpactLocation, FRotator::ZeroRotator, SpawnParameters))
		{
			DamageNumber->InitializeDamageNumber(Damage, bCritical);
		}
	}
	if (Settings->bEnableImpactSounds)
	{
		ChopItFeedbackAudio::PlayWoodImpact(this, GetOwner()->GetActorLocation(), bCritical, Settings->EffectsVolume);
	}
	if (bWoodenTarget && Settings->EffectsDensity > 0.0f)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector AwayFromImpact = DamageSource ? ImpactLocation - DamageSource->GetActorLocation() : FVector::ForwardVector;
		if (AChopItFeedbackBurst* Burst = World->SpawnActor<AChopItFeedbackBurst>(AChopItFeedbackBurst::StaticClass(), ImpactLocation, FRotator::ZeroRotator, SpawnParameters))
		{
			Burst->InitializeBurst(AwayFromImpact, bCritical, false, Settings->EffectsDensity);
		}
		if (FoliageComponent.IsValid())
		{
			if (AChopItLeafFall* Leaves = World->SpawnActor<AChopItLeafFall>(AChopItLeafFall::StaticClass(), FoliageComponent->GetComponentLocation(), FRotator::ZeroRotator, SpawnParameters))
			{
				Leaves->InitializeLeafFall(Settings->EffectsDensity, false);
			}
		}
	}
	if (Settings->bEnableCameraShake && CameraBoom.IsValid() && Settings->CameraShakeStrength > 0.0f)
	{
		const float Magnitude = (bCritical ? 16.0f : 9.0f) * Settings->CameraShakeStrength;
		CameraBoom->TargetOffset = OriginalCameraOffset + FVector(
			FMath::FRandRange(-Magnitude, Magnitude), FMath::FRandRange(-Magnitude, Magnitude), 0.0f);
		World->GetTimerManager().SetTimer(CameraTimerHandle, this, &UChopItHitFeedbackComponent::RestoreCamera, 0.07f, false);
	}
}

void UChopItHitFeedbackComponent::HandleDeath(AActor* DeadActor, AActor*)
{
	const UChopItDeveloperSettings* Settings = GetDefault<UChopItDeveloperSettings>();
	if (!Settings || Settings->EffectsDensity <= 0.0f || !GetWorld() || !DeadActor)
	{
		return;
	}
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AChopItFeedbackBurst* Burst = GetWorld()->SpawnActor<AChopItFeedbackBurst>(AChopItFeedbackBurst::StaticClass(), DeadActor->GetActorLocation(), FRotator::ZeroRotator, SpawnParameters))
	{
		Burst->InitializeBurst(FVector::UpVector, false, true, Settings->EffectsDensity);
	}
	if (FoliageComponent.IsValid())
	{
		if (AChopItLeafFall* Leaves = GetWorld()->SpawnActor<AChopItLeafFall>(AChopItLeafFall::StaticClass(), FoliageComponent->GetComponentLocation(), FRotator::ZeroRotator, SpawnParameters))
		{
			Leaves->InitializeLeafFall(Settings->EffectsDensity, true);
		}
	}
}

void UChopItHitFeedbackComponent::RestorePulse()
{
	if (VisualComponent.IsValid())
	{
		VisualComponent->SetRelativeScale3D(OriginalVisualScale);
	}
}

void UChopItHitFeedbackComponent::RestoreCamera()
{
	if (CameraBoom.IsValid())
	{
		CameraBoom->TargetOffset = OriginalCameraOffset;
	}
}
