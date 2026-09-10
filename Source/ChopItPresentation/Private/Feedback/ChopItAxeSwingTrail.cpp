#include "Feedback/ChopItAxeSwingTrail.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName RevealParameter(TEXT("Reveal"));
	const FName TailParameter(TEXT("Tail"));
	const FName OpacityParameter(TEXT("Opacity"));
	const FName EdgePowerParameter(TEXT("EdgePower"));
	const FName IntensityParameter(TEXT("Intensity"));
	const FName ColorParameter(TEXT("SlashColor"));
	const FName MaskScaleParameter(TEXT("MaskScale"));
	const FName MaskOffsetParameter(TEXT("MaskOffset"));
	const FName BreakupParameter(TEXT("Breakup"));
	const FName MaskWeightsParameter(TEXT("MaskWeights"));

	float LayerFade(const float NormalizedAge, const float FadeStart)
	{
		return NormalizedAge < FadeStart
			? 1.0f
			: 1.0f - FMath::SmoothStep(FadeStart, 1.0f, NormalizedAge);
	}

	void AnimateLayer(
		UMaterialInstanceDynamic* Material,
		const float NormalizedAge,
		const float RevealPortion,
		const float TailStart,
		const float FadeStart,
		const float OpacityScale)
	{
		if (!Material)
		{
			return;
		}
		const float Reveal = FMath::InterpEaseOut(
			-0.035f,
			1.035f,
			FMath::Clamp(NormalizedAge / RevealPortion, 0.0f, 1.0f),
			2.2f);
		const float Tail = NormalizedAge < TailStart
			? -0.06f
			: FMath::InterpEaseIn(
				-0.06f,
				1.06f,
				FMath::Clamp((NormalizedAge - TailStart) / (1.0f - TailStart), 0.0f, 1.0f),
				1.65f);
		Material->SetScalarParameterValue(RevealParameter, Reveal);
		Material->SetScalarParameterValue(TailParameter, Tail);
		Material->SetScalarParameterValue(
			OpacityParameter,
			LayerFade(NormalizedAge, FadeStart) * OpacityScale);
	}
}

AChopItAxeSwingTrail::AChopItAxeSwingTrail()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorEnableCollision(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	AfterimageLayer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AfterimageLayer"));
	AfterimageLayer->SetupAttachment(Root);
	MainLayer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainLayer"));
	MainLayer->SetupAttachment(Root);
	InnerLayer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InnerLayer"));
	InnerLayer->SetupAttachment(Root);
	DetailParticles = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DetailParticles"));
	DetailParticles->SetupAttachment(Root);
	DetailParticles->SetAutoActivate(false);
	DetailParticles->SetSystemFixedBounds(FBox(
		FVector(-650.0f, -650.0f, -100.0f),
		FVector(650.0f, 650.0f, 180.0f)));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> FallbackMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackMaterial(
		TEXT("/Engine/EngineMaterials/DefaultParticle.DefaultParticle"));

	MainMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(
		TEXT("/Game/ChopIt/Presentation/VFX/Meshes/SM_AxeSlash_Main.SM_AxeSlash_Main")));
	InnerMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(
		TEXT("/Game/ChopIt/Presentation/VFX/Meshes/SM_AxeSlash_Inner.SM_AxeSlash_Inner")));
	AfterimageMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(
		TEXT("/Game/ChopIt/Presentation/VFX/Meshes/SM_AxeSlash_Afterimage.SM_AxeSlash_Afterimage")));
	AdditiveMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
		TEXT("/Game/ChopIt/Presentation/VFX/Materials/M_AxeSlash.M_AxeSlash")));
	AfterimageMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
		TEXT("/Game/ChopIt/Presentation/VFX/Materials/M_AxeSlash_Afterimage.M_AxeSlash_Afterimage")));
	DetailsSystemAsset = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(
		TEXT("/Game/ChopIt/Presentation/VFX/Niagara/NS_AxeSlash_Details.NS_AxeSlash_Details")));

	for (UStaticMeshComponent* Layer : { AfterimageLayer.Get(), MainLayer.Get(), InnerLayer.Get() })
	{
		Layer->SetStaticMesh(FallbackMesh.Object);
		Layer->SetMaterial(0, FallbackMaterial.Object);
		Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Layer->SetGenerateOverlapEvents(false);
		Layer->SetCastShadow(false);
		Layer->SetReceivesDecals(false);
	}
	AfterimageLayer->SetTranslucentSortPriority(10);
	MainLayer->SetTranslucentSortPriority(11);
	InnerLayer->SetTranslucentSortPriority(12);
	AfterimageLayer->SetRelativeLocation(FVector(0.0f, 0.0f, -1.5f));
	MainLayer->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	InnerLayer->SetRelativeLocation(FVector(0.0f, 0.0f, 1.5f));
}

void AChopItAxeSwingTrail::InitializeTrail(
	const FVector& Forward,
	const float Range,
	const bool bHit,
	const float EffectsDensity)
{
	Density = FMath::Clamp(EffectsDensity, 0.0f, 1.0f);
	if (Density <= UE_KINDA_SMALL_NUMBER)
	{
		SetActorHiddenInGame(true);
		DetailParticles->DeactivateImmediate();
		SetLifeSpan(0.01f);
		return;
	}

	const FVector FlatForward = FVector(Forward.X, Forward.Y, 0.0f).GetSafeNormal(
		UE_SMALL_NUMBER,
		FVector::ForwardVector);
	SetActorLocation(GetActorLocation() + FVector(0.0f, 0.0f, 58.0f));
	BaseRotation = FlatForward.Rotation();
	SetActorRotation(BaseRotation);
	bWasHit = bHit;
	MainDuration = bHit ? 0.27f : 0.23f;
	InnerDuration = bHit ? 0.31f : 0.29f;
	AfterimageDuration = bHit ? 0.36f : 0.34f;
	Duration = AfterimageDuration;
	const float RangeScale = FMath::Max(0.35f, Range / 100.0f);
	BaseScale = FVector(RangeScale, RangeScale, 1.0f);
	SetActorScale3D(BaseScale);

	UStaticMesh* MainMesh = MainMeshAsset.LoadSynchronous();
	UStaticMesh* InnerMesh = InnerMeshAsset.LoadSynchronous();
	UStaticMesh* AfterimageMesh = AfterimageMeshAsset.LoadSynchronous();
	UMaterialInterface* AdditiveMaterial = AdditiveMaterialAsset.LoadSynchronous();
	UMaterialInterface* TranslucentMaterial = AfterimageMaterialAsset.LoadSynchronous();
	if (MainMesh)
	{
		MainLayer->SetStaticMesh(MainMesh);
	}
	if (InnerMesh)
	{
		InnerLayer->SetStaticMesh(InnerMesh);
	}
	if (AfterimageMesh)
	{
		AfterimageLayer->SetStaticMesh(AfterimageMesh);
	}
	if (AdditiveMaterial)
	{
		MainLayer->SetMaterial(0, AdditiveMaterial);
		InnerLayer->SetMaterial(0, AdditiveMaterial);
	}
	if (TranslucentMaterial)
	{
		AfterimageLayer->SetMaterial(0, TranslucentMaterial);
	}

	const float InnerDensity = FMath::Clamp((Density - 0.08f) / 0.92f, 0.0f, 1.0f);
	const float AfterimageDensity = FMath::Clamp((Density - 0.28f) / 0.72f, 0.0f, 1.0f);
	MainMaterial = ConfigureLayer(
		MainLayer,
		bHit ? FLinearColor(1.0f, 1.0f, 1.0f) : FLinearColor(0.93f, 0.96f, 1.0f),
		bHit ? 15.0f : 10.5f,
		0.72f,
		1.0f,
		1.45f,
		0.07f,
		0.30f,
		FLinearColor(0.78f, 0.12f, 0.04f, 0.06f));
	InnerMaterial = ConfigureLayer(
		InnerLayer,
		bHit ? FLinearColor(0.88f, 0.93f, 1.0f) : FLinearColor(0.63f, 0.69f, 0.78f),
		bHit ? 7.0f : 4.4f,
		1.65f,
		0.72f * InnerDensity,
		2.85f,
		0.31f,
		0.56f,
		FLinearColor(0.08f, 0.78f, 0.08f, 0.06f));
	AfterimageMaterial = ConfigureLayer(
		AfterimageLayer,
		FLinearColor(0.17f, 0.20f, 0.26f),
		bHit ? 1.35f : 0.85f,
		0.48f,
		(bHit ? 0.46f : 0.34f) * AfterimageDensity,
		1.95f,
		0.63f,
		0.68f,
		FLinearColor(0.05f, 0.10f, 0.78f, 0.07f));

	InnerLayer->SetVisibility(InnerDensity > UE_KINDA_SMALL_NUMBER);
	AfterimageLayer->SetVisibility(AfterimageDensity > UE_KINDA_SMALL_NUMBER);

	if (UNiagaraSystem* DetailsSystem = DetailsSystemAsset.LoadSynchronous())
	{
		const int32 GlintCount = FMath::RoundToInt((bHit ? 14.0f : 8.0f) * Density);
		const int32 FragmentCount = FMath::RoundToInt((bHit ? 9.0f : 5.0f) * Density);
		DetailParticles->SetAsset(DetailsSystem);
		DetailParticles->SetVariableInt(TEXT("User.GlintCount"), GlintCount);
		DetailParticles->SetVariableInt(TEXT("User.FragmentCount"), FragmentCount);
		DetailParticles->SetVariableFloat(TEXT("User.Range"), Range);
		DetailParticles->SetVariableFloat(TEXT("User.EffectsDensity"), Density);
		if (GlintCount + FragmentCount > 0)
		{
			DetailParticles->Activate(true);
		}
	}

	Age = 0.0f;
	UpdateTrail();
	SetLifeSpan(Duration);
	SetActorTickEnabled(true);
}

UMaterialInstanceDynamic* AChopItAxeSwingTrail::ConfigureLayer(
	UStaticMeshComponent* Layer,
	const FLinearColor& Color,
	const float Intensity,
	const float EdgePower,
	const float Opacity,
	const float MaskScale,
	const float MaskOffset,
	const float Breakup,
	const FLinearColor& MaskWeights)
{
	if (!Layer)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* DynamicMaterial = Layer->CreateAndSetMaterialInstanceDynamic(0);
	if (!DynamicMaterial)
	{
		return nullptr;
	}
	DynamicMaterial->SetVectorParameterValue(ColorParameter, Color);
	DynamicMaterial->SetScalarParameterValue(IntensityParameter, Intensity);
	DynamicMaterial->SetScalarParameterValue(EdgePowerParameter, EdgePower);
	DynamicMaterial->SetScalarParameterValue(OpacityParameter, Opacity);
	DynamicMaterial->SetScalarParameterValue(MaskScaleParameter, MaskScale);
	DynamicMaterial->SetScalarParameterValue(MaskOffsetParameter, MaskOffset);
	DynamicMaterial->SetScalarParameterValue(BreakupParameter, Breakup);
	DynamicMaterial->SetVectorParameterValue(MaskWeightsParameter, MaskWeights);
	return DynamicMaterial;
}

void AChopItAxeSwingTrail::UpdateTrail()
{
	const float MainAge = FMath::Clamp(Age / MainDuration, 0.0f, 1.0f);
	const float InnerAge = FMath::Clamp((Age - 0.012f) / InnerDuration, 0.0f, 1.0f);
	const float AfterimageAge = FMath::Clamp((Age - 0.025f) / AfterimageDuration, 0.0f, 1.0f);
	const float InnerDensity = FMath::Clamp((Density - 0.08f) / 0.92f, 0.0f, 1.0f);
	const float AfterimageDensity = FMath::Clamp((Density - 0.28f) / 0.72f, 0.0f, 1.0f);

	AnimateLayer(MainMaterial, MainAge, 0.43f, 0.28f, 0.58f, 1.0f);
	AnimateLayer(InnerMaterial, InnerAge, 0.50f, 0.34f, 0.62f, 0.72f * InnerDensity);
	AnimateLayer(
		AfterimageMaterial,
		AfterimageAge,
		0.58f,
		0.43f,
		0.69f,
		(bWasHit ? 0.46f : 0.34f) * AfterimageDensity);

	const float NormalizedAge = FMath::Clamp(Age / Duration, 0.0f, 1.0f);
	const float Expansion = 1.0f + FMath::Sin(NormalizedAge * UE_PI) * (bWasHit ? 0.065f : 0.038f);
	SetActorScale3D(BaseScale * FVector(Expansion, Expansion, 1.0f));
	SetActorRotation(BaseRotation + FRotator(0.0f, (bWasHit ? 7.0f : 4.0f) * NormalizedAge, 0.0f));
	AfterimageLayer->SetRelativeRotation(FRotator(0.0f, -4.0f - 4.0f * NormalizedAge, 0.0f));
	InnerLayer->SetRelativeRotation(FRotator(0.0f, 2.0f * NormalizedAge, 0.0f));
}

void AChopItAxeSwingTrail::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	UpdateTrail();
}
