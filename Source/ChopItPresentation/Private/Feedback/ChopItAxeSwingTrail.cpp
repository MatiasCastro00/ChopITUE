#include "Feedback/ChopItAxeSwingTrail.h"

#include "Components/SceneComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

AChopItAxeSwingTrail::AChopItAxeSwingTrail()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	SlashParticles = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SlashParticles"));
	SlashParticles->SetupAttachment(Root);
	SlashParticles->SetAutoActivate(false);
	SlashParticles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SlashParticles->SetGenerateOverlapEvents(false);
	// The mesh occupies local XY and opens toward -Y; gameplay forward is +X.
	SlashParticles->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	SlashParticles->SetSystemFixedBounds(FBox(FVector(-160.0f), FVector(160.0f)));
	SlashParticles->SetEmitterFixedBounds(TEXT("NE_SlashBits"), FBox(FVector(-160.0f), FVector(160.0f)));

	// A hard reference keeps the complete system and its dependencies in cooked builds.
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> AuthoredSlash(
		TEXT("/Game/ChopIt/Art/FX/NS_AxeSlash.NS_AxeSlash"));
	SlashSystem = AuthoredSlash.Object;
}

void AChopItAxeSwingTrail::InitializeTrail(
	const FVector& Forward,
	const float Range,
	const bool bHit,
	const float EffectsDensity)
{
	const float Density = FMath::Clamp(EffectsDensity, 0.0f, 1.0f);
	if (Density <= UE_KINDA_SMALL_NUMBER || !SlashSystem)
	{
		SetActorHiddenInGame(true);
		SlashParticles->DeactivateImmediate();
		SetLifeSpan(0.01f);
		return;
	}

	const FVector FlatForward = FVector(Forward.X, Forward.Y, 0.0f).GetSafeNormal(
		UE_SMALL_NUMBER, FVector::ForwardVector);
	SetActorLocation(GetActorLocation() + FVector(0.0f, 0.0f, 58.0f));
	SetActorRotation(FlatForward.Rotation());
	// Authored mesh radius is 112 cm, measured from its pivot.
	SetActorScale3D(FVector(FMath::Max(0.35f, Range / 112.0f)));
	SlashParticles->SetAsset(SlashSystem);
	SlashParticles->SetEmitterEnable(TEXT("NE_SlashEdge"), Density > 0.08f);
	SlashParticles->SetEmitterEnable(TEXT("NE_SlashBits"), Density > 0.28f);
	SlashParticles->Activate(true);

	// Last sparks spawn at 0.28 s and live up to 0.20 s. End before the 1 s loop.
	SetLifeSpan(0.6f);
}
