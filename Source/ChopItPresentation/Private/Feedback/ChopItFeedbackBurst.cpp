#include "Feedback/ChopItFeedbackBurst.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AChopItFeedbackBurst::AChopItFeedbackBurst()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorEnableCollision(false);
	BarkFragments = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BarkFragments"));
	SetRootComponent(BarkFragments);
	BarkFragments->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BarkMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodMaterial(TEXT("/Game/ChopIt/World/Blockout/Materials/MI_Wood.MI_Wood"));
	if (BarkMesh.Succeeded()) { BarkFragments->SetStaticMesh(BarkMesh.Object); }
	if (WoodMaterial.Succeeded()) { BarkFragments->SetMaterial(0, WoodMaterial.Object); }
}

void AChopItFeedbackBurst::InitializeBurst(const FVector& AwayFromImpact, const bool bCritical, const bool bDeathBurst, const float Density)
{
	const int32 BaseCount = bDeathBurst ? 14 : (bCritical ? 8 : 5);
	const int32 Count = FMath::Clamp(FMath::RoundToInt(BaseCount * Density), 0, 18);
	if (Count == 0)
	{
		Destroy();
		return;
	}
	const FVector Direction = AwayFromImpact.GetSafeNormal2D(UE_SMALL_NUMBER, FVector::ForwardVector);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FFragmentState State;
		State.Location = GetActorLocation() + FVector(0.0f, 0.0f, FMath::FRandRange(-12.0f, 18.0f));
		State.Velocity = Direction * FMath::FRandRange(180.0f, bDeathBurst ? 430.0f : 290.0f)
			+ FVector(FMath::FRandRange(-130.0f, 130.0f), FMath::FRandRange(-130.0f, 130.0f), FMath::FRandRange(150.0f, 440.0f));
		State.Rotation = FRotator(FMath::FRandRange(0.0f, 360.0f), FMath::FRandRange(0.0f, 360.0f), FMath::FRandRange(0.0f, 360.0f));
		State.Scale = FVector(FMath::FRandRange(0.05f, 0.16f));
		State.InstanceIndex = BarkFragments->AddInstance(FTransform(State.Rotation, State.Location, State.Scale), true);
		Fragments.Add(State);
	}
	SetLifeSpan(bDeathBurst ? 1.25f : 0.7f);
	SetActorTickEnabled(true);
}

void AChopItFeedbackBurst::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	for (FFragmentState& Fragment : Fragments)
	{
		Fragment.Velocity.Z -= 1200.0f * DeltaSeconds;
		Fragment.Velocity *= FMath::Pow(0.12f, DeltaSeconds);
		Fragment.Location += Fragment.Velocity * DeltaSeconds;
		Fragment.Rotation += FRotator(420.0f * DeltaSeconds, 620.0f * DeltaSeconds, 310.0f * DeltaSeconds);
		BarkFragments->UpdateInstanceTransform(Fragment.InstanceIndex, FTransform(Fragment.Rotation, Fragment.Location, Fragment.Scale), true, false, true);
	}
}
