#include "Feedback/ChopItAxeSwingTrail.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AChopItAxeSwingTrail::AChopItAxeSwingTrail()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorEnableCollision(false);
	SlashMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlashMesh"));
	SetRootComponent(SlashMesh);
	SlashMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlayerMaterial(TEXT("/Game/ChopIt/World/Blockout/Materials/MI_Player.MI_Player"));
	if (ConeMesh.Succeeded()) { SlashMesh->SetStaticMesh(ConeMesh.Object); }
	if (PlayerMaterial.Succeeded()) { SlashMesh->SetMaterial(0, PlayerMaterial.Object); }
}

void AChopItAxeSwingTrail::InitializeTrail(const FVector& Forward, const float Range, const bool bHit)
{
	const FVector FlatForward = FVector(Forward.X, Forward.Y, 0.0f).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	SetActorLocation(GetActorLocation() + FlatForward * Range * 0.38f + FVector(0.0f, 0.0f, 50.0f));
	SetActorRotation(FlatForward.Rotation() + FRotator(0.0f, 0.0f, 90.0f));
	InitialScale = FVector(bHit ? 0.22f : 0.16f, Range / 420.0f, 0.05f);
	SlashMesh->SetRelativeScale3D(InitialScale);
	SetLifeSpan(0.16f);
	SetActorTickEnabled(true);
}

void AChopItAxeSwingTrail::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float Scale = 1.0f + Age * 2.2f;
	SlashMesh->SetRelativeScale3D(InitialScale * Scale);
}
