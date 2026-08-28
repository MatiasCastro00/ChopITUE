#include "Feedback/ChopItLeafFall.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AChopItLeafFall::AChopItLeafFall()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorEnableCollision(false);
	Leaves = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Leaves"));
	SetRootComponent(Leaves);
	Leaves->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LeafMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LeavesMaterial(TEXT("/Game/ChopIt/World/Blockout/Materials/MI_Leaves.MI_Leaves"));
	if (LeafMesh.Succeeded()) { Leaves->SetStaticMesh(LeafMesh.Object); }
	if (LeavesMaterial.Succeeded()) { Leaves->SetMaterial(0, LeavesMaterial.Object); }
}

void AChopItLeafFall::InitializeLeafFall(const float Density, const bool bHeavyFall)
{
	bHeavy = bHeavyFall;
	RemainingLeaves = FMath::Clamp(FMath::RoundToInt((bHeavy ? 18.0f : 7.0f) * Density), 0, 24);
	if (RemainingLeaves == 0)
	{
		Destroy();
		return;
	}
	SetLifeSpan(bHeavy ? 2.0f : 1.25f);
	SetActorTickEnabled(true);
}

void AChopItLeafFall::SpawnLeaf()
{
	FLeafState Leaf;
	Leaf.Location = GetActorLocation() + FVector(FMath::FRandRange(-160.0f, 160.0f), FMath::FRandRange(-160.0f, 160.0f), FMath::FRandRange(-70.0f, 85.0f));
	Leaf.DriftDirection = FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), 0.0f).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	Leaf.Phase = FMath::FRandRange(0.0f, 2.0f * PI);
	Leaf.FallSpeed = FMath::FRandRange(bHeavy ? 95.0f : 55.0f, bHeavy ? 175.0f : 110.0f);
	Leaf.Rotation = FRotator(FMath::FRandRange(0.0f, 360.0f), FMath::FRandRange(0.0f, 360.0f), FMath::FRandRange(0.0f, 360.0f));
	Leaf.Scale = FVector(FMath::FRandRange(0.07f, 0.16f), FMath::FRandRange(0.18f, 0.34f), 0.045f);
	Leaf.InstanceIndex = Leaves->AddInstance(FTransform(Leaf.Rotation, Leaf.Location, Leaf.Scale), true);
	ActiveLeaves.Add(Leaf);
	--RemainingLeaves;
}

void AChopItLeafFall::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	SpawnAccumulator += DeltaSeconds;
	const float SpawnInterval = bHeavy ? 0.045f : 0.11f;
	while (RemainingLeaves > 0 && SpawnAccumulator >= SpawnInterval)
	{
		SpawnAccumulator -= SpawnInterval;
		SpawnLeaf();
	}
	for (FLeafState& Leaf : ActiveLeaves)
	{
		const float Sway = FMath::Sin(Age * 5.5f + Leaf.Phase) * 55.0f;
		Leaf.Location += (Leaf.DriftDirection * (45.0f + Sway) + FVector(0.0f, 0.0f, -Leaf.FallSpeed)) * DeltaSeconds;
		Leaf.Rotation += FRotator(160.0f * DeltaSeconds, 250.0f * DeltaSeconds, 135.0f * DeltaSeconds);
		Leaves->UpdateInstanceTransform(Leaf.InstanceIndex, FTransform(Leaf.Rotation, Leaf.Location, Leaf.Scale), true, false, true);
	}
}
