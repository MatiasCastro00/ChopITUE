#include "Cycle/ChopItFireflySwarm.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AChopItFireflySwarm::AChopItFireflySwarm()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f / 30.0f;
	SetActorEnableCollision(false);

	FireflyInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FireflyInstances"));
	SetRootComponent(FireflyInstances);
	FireflyInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FireflyInstances->SetCastShadow(false);
	FireflyInstances->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		FireflyInstances->SetStaticMesh(SphereMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveMaterial(
		TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (EmissiveMaterial.Succeeded())
	{
		FireflyInstances->SetMaterial(0, EmissiveMaterial.Object);
	}
}

void AChopItFireflySwarm::Configure(
	const int32 InCount,
	const float InHorizontalRadius,
	const float InMaximumHeight)
{
	FireflyCount = FMath::Clamp(InCount, 1, 128);
	HorizontalRadius = FMath::Max(100.0f, InHorizontalRadius);
	MaximumHeight = FMath::Max(100.0f, InMaximumHeight);
	if (HasActorBegunPlay())
	{
		PopulateSwarm();
	}
}

void AChopItFireflySwarm::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* BaseMaterial = FireflyInstances->GetMaterial(0))
	{
		UMaterialInstanceDynamic* GlowMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		GlowMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.58f, 0.04f, 1.0f));
		GlowMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(1.0f, 0.58f, 0.04f, 1.0f));
		FireflyInstances->SetMaterial(0, GlowMaterial);
	}
	PopulateSwarm();
}

void AChopItFireflySwarm::PopulateSwarm()
{
	FireflyInstances->ClearInstances();
	Fireflies.Reset(FireflyCount);
	Fireflies.Reserve(FireflyCount);
	FRandomStream Random(0xF1AEF17);
	for (int32 Index = 0; Index < FireflyCount; ++Index)
	{
		FFireflyState& Firefly = Fireflies.AddDefaulted_GetRef();
		const float Angle = Random.FRandRange(0.0f, UE_TWO_PI);
		const float Radius = FMath::Sqrt(Random.FRand()) * HorizontalRadius;
		Firefly.Position = FVector(
			FMath::Cos(Angle) * Radius,
			FMath::Sin(Angle) * Radius,
			Random.FRandRange(55.0f, MaximumHeight));
		Firefly.Phase = Random.FRandRange(0.0f, UE_TWO_PI);
		Firefly.FlightRate = Random.FRandRange(0.55f, 1.35f);
		Firefly.PulseRate = Random.FRandRange(1.6f, 3.2f);
		const float InitialScale = Random.FRandRange(0.035f, 0.065f);
		FireflyInstances->AddInstance(FTransform(FQuat::Identity, Firefly.Position, FVector(InitialScale)));
	}
}

void AChopItFireflySwarm::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	for (int32 Index = 0; Index < Fireflies.Num(); ++Index)
	{
		FFireflyState& Firefly = Fireflies[Index];
		Firefly.Phase += DeltaSeconds * Firefly.FlightRate;
		const FVector DesiredVelocity(
			FMath::Sin(Firefly.Phase * 1.13f + Index * 0.71f) * 42.0f,
			FMath::Cos(Firefly.Phase * 0.87f + Index * 1.17f) * 42.0f,
			FMath::Sin(Firefly.Phase * 1.61f + Index) * 20.0f);
		Firefly.Velocity = FMath::VInterpTo(Firefly.Velocity, DesiredVelocity, DeltaSeconds, 2.2f);
		Firefly.Position += Firefly.Velocity * DeltaSeconds;

		const FVector2D Horizontal(Firefly.Position.X, Firefly.Position.Y);
		if (Horizontal.SizeSquared() > FMath::Square(HorizontalRadius))
		{
			const FVector2D Clamped = Horizontal.GetSafeNormal() * HorizontalRadius;
			Firefly.Position.X = Clamped.X;
			Firefly.Position.Y = Clamped.Y;
			Firefly.Velocity.X *= -0.65f;
			Firefly.Velocity.Y *= -0.65f;
		}
		if (Firefly.Position.Z < 45.0f || Firefly.Position.Z > MaximumHeight)
		{
			Firefly.Position.Z = FMath::Clamp(Firefly.Position.Z, 45.0f, MaximumHeight);
			Firefly.Velocity.Z *= -0.75f;
		}

		const float Pulse = 0.045f + 0.018f * (0.5f + 0.5f * FMath::Sin(Time * Firefly.PulseRate + Index));
		const bool bMarkRenderStateDirty = Index == Fireflies.Num() - 1;
		FireflyInstances->UpdateInstanceTransform(
			Index,
			FTransform(FQuat::Identity, Firefly.Position, FVector(Pulse)),
			false,
			bMarkRenderStateDirty,
			true);
	}
}
