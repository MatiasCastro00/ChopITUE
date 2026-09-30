#include "Economy/ChopItDeathRemains.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AChopItDeathRemains::AChopItDeathRemains()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterial(
		TEXT("/Game/ChopIt/World/Blockout/Materials/MI_Wood.MI_Wood"));
	BloodDropletMesh = SphereMesh.Object;
	MeatFragmentMesh = CubeMesh.Object;
	FragmentBaseMaterial = BaseMaterial.Object;
}

void AChopItDeathRemains::InitializeRemains(const FVector& EjectionDirection)
{
	if (!BloodDropletMesh || !MeatFragmentMesh) return;

	if (FragmentBaseMaterial)
	{
		BloodMaterial = UMaterialInstanceDynamic::Create(FragmentBaseMaterial, this);
		MeatMaterial = UMaterialInstanceDynamic::Create(FragmentBaseMaterial, this);
		if (BloodMaterial) BloodMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.34f, 0.004f, 0.002f));
		if (MeatMaterial) MeatMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.24f, 0.025f, 0.012f));
	}

	FRandomStream Random(GetUniqueID() * 196613 + 97);
	const FVector Direction = EjectionDirection.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	// A single terminal death leaves these components in the world. Chaos lets
	// them bounce and settle naturally instead of fading mid-air like hit VFX.
	for (int32 Index = 0; Index < 22; ++Index)
	{
		const bool bBlood = Index < 14;
		UStaticMeshComponent* Fragment = NewObject<UStaticMeshComponent>(this);
		Fragment->SetStaticMesh(bBlood ? BloodDropletMesh : MeatFragmentMesh);
		if (bBlood && BloodMaterial) Fragment->SetMaterial(0, BloodMaterial);
		if (!bBlood && MeatMaterial) Fragment->SetMaterial(0, MeatMaterial);
		Fragment->SetupAttachment(SceneRoot);
		Fragment->SetCollisionProfileName(TEXT("PhysicsActor"));
		Fragment->SetGenerateOverlapEvents(false);
		Fragment->SetCanEverAffectNavigation(false);
		Fragment->RegisterComponent();

		const float Size = bBlood ? Random.FRandRange(0.045f, 0.12f) : Random.FRandRange(0.07f, 0.16f);
		Fragment->SetWorldScale3D(bBlood
			? FVector(Size, Size, Size * Random.FRandRange(0.20f, 0.50f))
			: FVector(Size * Random.FRandRange(0.75f, 1.35f), Size, Size * Random.FRandRange(0.70f, 1.45f)));
		Fragment->SetWorldLocation(GetActorLocation() + FVector(
			Random.FRandRange(-24.0f, 24.0f), Random.FRandRange(-24.0f, 24.0f), Random.FRandRange(5.0f, 48.0f)));
		Fragment->SetSimulatePhysics(true);
		Fragment->SetEnableGravity(true);
		Fragment->SetLinearDamping(0.55f);
		Fragment->SetAngularDamping(1.2f);
		Fragment->SetMassOverrideInKg(NAME_None, bBlood ? 0.08f : 0.32f, true);
		const FVector Scatter(Random.FRandRange(-0.75f, 0.75f), Random.FRandRange(-0.75f, 0.75f), Random.FRandRange(0.25f, 1.0f));
		Fragment->AddImpulse((Direction * Random.FRandRange(180.0f, 430.0f) + Scatter.GetSafeNormal() * Random.FRandRange(160.0f, 390.0f)), NAME_None, true);
	}
}
