#include "Harvest/ChopItTree.h"

#include "ChopItCollision.h"
#include "ChopItDeveloperSettings.h"
#include "ChopItLogChannels.h"
#include "Combat/ChopItHealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Harvest/ChopItForestRegistrySubsystem.h"
#include "Harvest/ChopItLogPickup.h"
#include "Feedback/ChopItHitFeedbackComponent.h"
#include "Feedback/ChopItFeedbackBurst.h"
#include "Feedback/ChopItLeafFall.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AChopItTree::AChopItTree()
{
	PrimaryActorTick.bCanEverTick = false;

	PhysicsRoot = CreateDefaultSubobject<UBoxComponent>(TEXT("PhysicsRoot"));
	SetRootComponent(PhysicsRoot);
	PhysicsRoot->InitBoxExtent(FVector(55.0f, 55.0f, 380.0f));
	PhysicsRoot->SetCollisionProfileName(ChopItCollisionProfiles::Harvestable);
	PhysicsRoot->SetMobility(EComponentMobility::Movable);
	PhysicsRoot->SetSimulatePhysics(false);
	PhysicsRoot->SetNotifyRigidBodyCollision(true);
	PhysicsRoot->SetLinearDamping(0.45f);
	PhysicsRoot->SetAngularDamping(1.2f);

	TrunkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrunkMesh"));
	TrunkMesh->SetupAttachment(PhysicsRoot);
	TrunkMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -190.0f));
	TrunkMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 3.8f));

	CrownMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrownMesh"));
	CrownMesh->SetupAttachment(PhysicsRoot);
	CrownMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	CrownMesh->SetRelativeScale3D(FVector(2.2f, 2.2f, 2.7f));

	// A separate query shape represents the leaves. The trunk remains the rigid
	// body that makes the tree fall, but only contact with this volume is allowed
	// to resolve the harvest and release the drops.
	CrownCollision = CreateDefaultSubobject<USphereComponent>(TEXT("CrownCollision"));
	CrownCollision->SetupAttachment(PhysicsRoot);
	CrownCollision->InitSphereRadius(125.0f);
	CrownCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	CrownCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CrownCollision->SetCollisionObjectType(ChopItCollisionChannels::Harvestable);
	CrownCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	CrownCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CrownCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CrownCollision->SetCollisionResponseToChannel(ChopItCollisionChannels::Harvestable, ECR_Block);
	CrownCollision->SetGenerateOverlapEvents(false);
	ConfigureCameraOcclusion();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (CylinderMesh.Succeeded())
	{
		TrunkMesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (SphereMesh.Succeeded())
	{
		CrownMesh->SetStaticMesh(SphereMesh.Object);
	}

	HealthLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthLabel"));
	HealthLabel->SetupAttachment(PhysicsRoot);
	HealthLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 400.0f));
	HealthLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	HealthLabel->SetHorizontalAlignment(EHTA_Center);
	HealthLabel->SetWorldSize(48.0f);
	HealthLabel->SetTextRenderColor(FColor::Green);

	HealthComponent = CreateDefaultSubobject<UChopItHealthComponent>(TEXT("HealthComponent"));
	HitFeedbackComponent = CreateDefaultSubobject<UChopItHitFeedbackComponent>(TEXT("HitFeedbackComponent"));
	HitFeedbackComponent->SetVisualComponent(PhysicsRoot);
	HitFeedbackComponent->SetWoodenTarget(true);
	HitFeedbackComponent->SetFoliageComponent(CrownMesh);
	// Tree death is resolved on its physical impact, not when health reaches zero.
	HitFeedbackComponent->SetDeathEffectsEnabled(false);
	LogPickupClass = AChopItLogPickup::StaticClass();
}

void AChopItTree::BeginPlay()
{
	Super::BeginPlay();
	// Reapply these responses at runtime so existing Blueprint defaults created
	// before the camera channels were introduced cannot make trees push the camera.
	ConfigureCameraOcclusion();
	HealthComponent->OnHealthChanged.AddUObject(this, &AChopItTree::HandleHealthChanged);
	HealthComponent->OnDeath.AddUObject(this, &AChopItTree::HandleDepleted);
	PhysicsRoot->OnComponentHit.AddDynamic(this, &AChopItTree::HandleFallImpact);
	ApplyFoliageColor();
	UpdateHealthLabel(HealthComponent->GetCurrentHealth(), HealthComponent->GetMaxHealth());
	if (UWorld* World = GetWorld())
	{
		if (UChopItForestRegistrySubsystem* Registry = World->GetSubsystem<UChopItForestRegistrySubsystem>())
		{
			Registry->RegisterTree(this);
		}
	}
}

void AChopItTree::ConfigureCameraOcclusion()
{
	// The physical root still blocks pawns, weapons and the world, but it must not
	// participate in either camera query: its box includes most of the crown.
	PhysicsRoot->SetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid, ECR_Ignore);
	PhysicsRoot->SetCollisionResponseToChannel(ChopItCollisionChannels::CameraOcclusion, ECR_Ignore);

	// Keep the rendered primitives in the query scene so ChopIt's final-pose
	// camera corridor can replace and later restore their materials.
	for (UStaticMeshComponent* VisualMesh : { TrunkMesh.Get(), CrownMesh.Get() })
	{
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		VisualMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		VisualMesh->SetCollisionResponseToChannel(ChopItCollisionChannels::CameraOcclusion, ECR_Block);
	}

	// This volume is exclusively for harvest/fall contact. Hitting it would apply
	// transparency to a component with no material instead of to the visible tree.
	CrownCollision->SetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid, ECR_Ignore);
	CrownCollision->SetCollisionResponseToChannel(ChopItCollisionChannels::CameraOcclusion, ECR_Ignore);
}

void AChopItTree::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FallSettleTimerHandle);
		if (UChopItForestRegistrySubsystem* Registry = World->GetSubsystem<UChopItForestRegistrySubsystem>())
		{
			Registry->UnregisterTree(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AChopItTree::SetBlockoutMaterials(UMaterialInterface* TrunkMaterial, UMaterialInterface* CrownMaterial)
{
	TrunkMesh->SetMaterial(0, TrunkMaterial);
	CrownMaterialSource = CrownMaterial;
	ApplyFoliageColor();
}

void AChopItTree::SetFoliageVariant(const EChopItTreeFoliageVariant NewVariant)
{
	FoliageVariant = NewVariant;
	ApplyFoliageColor();
}

FLinearColor AChopItTree::GetFoliageColor() const
{
	EChopItTreeFoliageVariant ResolvedVariant = FoliageVariant;
	if (ResolvedVariant == EChopItTreeFoliageVariant::Auto)
	{
		constexpr EChopItTreeFoliageVariant AutoVariants[] =
		{
			EChopItTreeFoliageVariant::Pine,
			EChopItTreeFoliageVariant::Spring,
			EChopItTreeFoliageVariant::Summer,
			EChopItTreeFoliageVariant::Autumn
		};
		ResolvedVariant = AutoVariants[GetTypeHash(GetFName()) % UE_ARRAY_COUNT(AutoVariants)];
	}

	switch (ResolvedVariant)
	{
	case EChopItTreeFoliageVariant::Pine:
		return FLinearColor(0.015f, 0.12f, 0.025f);
	case EChopItTreeFoliageVariant::Spring:
		return FLinearColor(0.14f, 0.52f, 0.045f);
	case EChopItTreeFoliageVariant::Summer:
		return FLinearColor(0.035f, 0.29f, 0.055f);
	case EChopItTreeFoliageVariant::Autumn:
		return FLinearColor(0.62f, 0.13f, 0.015f);
	default:
		return FLinearColor(0.015f, 0.12f, 0.025f);
	}
}

void AChopItTree::ApplyFoliageColor()
{
	if (!CrownMesh)
	{
		return;
	}

	if (!CrownMaterialSource)
	{
		CrownMaterialSource = CrownMesh->GetMaterial(0);
	}
	if (!CrownMaterialSource)
	{
		return;
	}

	CrownMaterialInstance = UMaterialInstanceDynamic::Create(CrownMaterialSource, this);
	if (CrownMaterialInstance)
	{
		CrownMaterialInstance->SetVectorParameterValue(TEXT("Color"), GetFoliageColor());
		CrownMesh->SetMaterial(0, CrownMaterialInstance);
	}
}

void AChopItTree::HandleHealthChanged(const float CurrentHealth, const float MaxHealth, AActor* DamageSource)
{
	UpdateHealthLabel(CurrentHealth, MaxHealth);
	UE_LOG(LogChopIt, Display, TEXT("Tree %s health: %.0f / %.0f"), *GetName(), CurrentHealth, MaxHealth);
}

void AChopItTree::HandleDepleted(AActor* DeadActor, AActor* DamageSource)
{
	if (HarvestState != EChopItTreeHarvestState::Standing)
	{
		return;
	}

	HarvestState = EChopItTreeHarvestState::Falling;
	RewardRecipient = DamageSource;
	HealthLabel->SetVisibility(false);
	PhysicsRoot->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PhysicsRoot->SetSimulatePhysics(true);
	PhysicsRoot->WakeAllRigidBodies();
	PreviousCrownLocation = CrownCollision->GetComponentLocation();

	FVector FallDirection = DamageSource
		? GetActorLocation() - DamageSource->GetActorLocation()
		: GetActorForwardVector();
	FallDirection.Z = 0.0f;
	FallDirection = FallDirection.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const FVector FallAxis(-FallDirection.Y, FallDirection.X, 0.0f);
	PhysicsRoot->SetPhysicsAngularVelocityInDegrees(FallAxis * 85.0f, false);
	PhysicsRoot->AddImpulse(FallDirection * 18000.0f, NAME_None, false);

	FallStartedAt = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(
		FallSettleTimerHandle,
		this,
		&AChopItTree::CheckFallSettled,
		0.025f,
		true,
		0.025f);
	UE_LOG(LogChopIt, Display, TEXT("Tree %s entered Falling."), *GetName());
}

void AChopItTree::HandleFallImpact(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if ((HarvestState != EChopItTreeHarvestState::Falling && HarvestState != EChopItTreeHarvestState::Settled)
		|| bRewardSpawned || !GetWorld())
	{
		return;
	}
	if (!IsImpactOnCrown(Hit))
	{
		return;
	}

	// The standing trunk already touches the floor. Ignore that initial contact
	// and emit the drop only once the falling motion has actually started.
	const double Elapsed = GetWorld()->GetTimeSeconds() - FallStartedAt;
	if (Elapsed < 0.15)
	{
		return;
	}

	FVector ImpactNormal = Hit.ImpactNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	if (ImpactNormal.Z < 0.15f)
	{
		ImpactNormal.Z = 0.15f;
		ImpactNormal.Normalize();
	}
	const FVector SpawnOrigin = Hit.ImpactPoint + ImpactNormal * 42.0f + FVector(0.0f, 0.0f, 24.0f);
	SpawnRewardOnce(SpawnOrigin, ImpactNormal);
	DestroyAtImpact(Hit.ImpactPoint, ImpactNormal);
	UE_LOG(LogChopIt, Display, TEXT("Tree %s released its reward on impact with %s."), *GetName(), *GetNameSafe(OtherActor));
}

void AChopItTree::CheckFallSettled()
{
	if (HarvestState != EChopItTreeHarvestState::Falling && HarvestState != EChopItTreeHarvestState::Settled)
	{
		GetWorldTimerManager().ClearTimer(FallSettleTimerHandle);
		return;
	}

	if (TryResolveCrownContact())
	{
		return;
	}

	if (HarvestState == EChopItTreeHarvestState::Settled)
	{
		return;
	}

	// Never harvest due only to a timeout or because the trunk stopped moving.
	// A resting tree remains and keeps a low-frequency crown-contact check so a
	// later push can still complete the harvest through the leaves.
	const double Elapsed = GetWorld()->GetTimeSeconds() - FallStartedAt;
	const bool bMotionSettled = PhysicsRoot->GetPhysicsLinearVelocity().SizeSquared() < FMath::Square(15.0)
		&& PhysicsRoot->GetPhysicsAngularVelocityInDegrees().SizeSquared() < FMath::Square(8.0);
	if (bMotionSettled || Elapsed >= MaximumFallDuration)
	{
		HarvestState = EChopItTreeHarvestState::Settled;
		GetWorldTimerManager().SetTimer(
			FallSettleTimerHandle,
			this,
			&AChopItTree::CheckFallSettled,
			0.1f,
			true);
	}
}

bool AChopItTree::TryResolveCrownContact()
{
	UWorld* World = GetWorld();
	if (!World || !CrownCollision
		|| (HarvestState != EChopItTreeHarvestState::Falling && HarvestState != EChopItTreeHarvestState::Settled)
		|| bRewardSpawned)
	{
		return false;
	}

	const FVector CurrentCrownLocation = CrownCollision->GetComponentLocation();
	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQuery.AddObjectTypesToQuery(ChopItCollisionChannels::Harvestable);
	FCollisionQueryParams QueryParams(FName(TEXT("TreeCrownContact")), false, this);
	QueryParams.bFindInitialOverlaps = true;
	FHitResult Hit;
	const bool bHit = World->SweepSingleByObjectType(
		Hit,
		PreviousCrownLocation,
		CurrentCrownLocation,
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(CrownCollision->GetScaledSphereRadius()),
		QueryParams);
	PreviousCrownLocation = CurrentCrownLocation;
	if (!bHit || !Hit.bBlockingHit)
	{
		return false;
	}

	FVector ImpactNormal = Hit.ImpactNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	if (ImpactNormal.Z < 0.15f)
	{
		ImpactNormal.Z = 0.15f;
		ImpactNormal.Normalize();
	}
	const FVector ImpactPoint = Hit.ImpactPoint.IsNearlyZero()
		? CurrentCrownLocation - ImpactNormal * CrownCollision->GetScaledSphereRadius()
		: Hit.ImpactPoint;
	const FVector SpawnOrigin = ImpactPoint + ImpactNormal * 42.0f + FVector(0.0f, 0.0f, 24.0f);
	SpawnRewardOnce(SpawnOrigin, ImpactNormal);
	DestroyAtImpact(ImpactPoint, ImpactNormal);
	UE_LOG(LogChopIt, Display, TEXT("Tree %s released its reward after verified crown contact with %s."), *GetName(), *GetNameSafe(Hit.GetActor()));
	return true;
}

bool AChopItTree::IsImpactOnCrown(const FHitResult& Hit) const
{
	if (!CrownCollision)
	{
		return false;
	}
	const float AllowedRadius = CrownCollision->GetScaledSphereRadius() + 12.0f;
	return FVector::DistSquared(Hit.ImpactPoint, CrownCollision->GetComponentLocation()) <= FMath::Square(AllowedRadius);
}

void AChopItTree::SpawnRewardOnce(const FVector& SpawnOrigin, const FVector& ImpactNormal)
{
	if (bRewardSpawned)
	{
		return;
	}
	bRewardSpawned = true;

	if (LogPickupClass)
	{
		const int32 PickupCount = FMath::Max(1, WoodRewardUnits);
		for (int32 Index = 0; Index < PickupCount; ++Index)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const FVector HorizontalScatter(
				FMath::FRandRange(-1.0f, 1.0f),
				FMath::FRandRange(-1.0f, 1.0f),
				0.0f);
			FVector LaunchDirection = ImpactNormal * 0.35f + HorizontalScatter;
			LaunchDirection.Z = FMath::Max(0.45f, LaunchDirection.Z + FMath::FRandRange(0.25f, 0.6f));
			LaunchDirection.Normalize();
			const FVector SpawnLocation = SpawnOrigin + HorizontalScatter.GetSafeNormal() * (Index * 8.0f);
			AChopItLogPickup* Pickup = GetWorld()->SpawnActor<AChopItLogPickup>(
				LogPickupClass,
				SpawnLocation,
				FRotator::ZeroRotator,
				SpawnParameters);
			if (Pickup)
			{
				Pickup->InitializeReward(1, Index == 0 ? ExperienceReward : 0);
				Pickup->LaunchFromImpact(
					LaunchDirection * FMath::FRandRange(180.0f, 280.0f),
					FVector(
						FMath::FRandRange(-240.0f, 240.0f),
						FMath::FRandRange(-240.0f, 240.0f),
						FMath::FRandRange(-320.0f, 320.0f)));
			}
		}
	}

	OnHarvested.Broadcast(this, WoodRewardUnits, ExperienceReward);
	UE_LOG(LogChopIt, Display, TEXT("Tree %s emitted one reward: wood=%d xp=%d."), *GetName(), WoodRewardUnits, ExperienceReward);
	SetLifeSpan(8.0f);
}

void AChopItTree::DestroyAtImpact(const FVector& ImpactLocation, const FVector& ImpactNormal)
{
	if (!GetWorld())
	{
		Destroy();
		return;
	}

	const UChopItDeveloperSettings* Settings = GetDefault<UChopItDeveloperSettings>();
	const float Density = Settings ? Settings->EffectsDensity : 1.0f;
	if (Density > 0.0f)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AChopItFeedbackBurst* Burst = GetWorld()->SpawnActor<AChopItFeedbackBurst>(
			AChopItFeedbackBurst::StaticClass(), ImpactLocation, FRotator::ZeroRotator, SpawnParameters))
		{
			Burst->InitializeBurst(ImpactNormal, false, true, Density);
		}
		if (CrownMesh)
		{
			if (AChopItLeafFall* Leaves = GetWorld()->SpawnActor<AChopItLeafFall>(
				AChopItLeafFall::StaticClass(), CrownMesh->GetComponentLocation(), FRotator::ZeroRotator, SpawnParameters))
			{
				Leaves->InitializeLeafFall(Density, true, GetFoliageColor());
			}
		}
	}

	GetWorldTimerManager().ClearTimer(FallSettleTimerHandle);
	PhysicsRoot->SetSimulatePhysics(false);
	PhysicsRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HarvestState = EChopItTreeHarvestState::Harvested;
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	Destroy();
}

void AChopItTree::UpdateHealthLabel(const float CurrentHealth, const float MaxHealth)
{
	HealthLabel->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), CurrentHealth, MaxHealth)));
}
