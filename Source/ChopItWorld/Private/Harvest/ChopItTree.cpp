#include "Harvest/ChopItTree.h"

#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "Combat/ChopItHealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Harvest/ChopItForestRegistrySubsystem.h"
#include "Harvest/ChopItLogPickup.h"
#include "Feedback/ChopItHitFeedbackComponent.h"
#include "Materials/MaterialInterface.h"
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
	PhysicsRoot->SetLinearDamping(0.45f);
	PhysicsRoot->SetAngularDamping(1.2f);

	TrunkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrunkMesh"));
	TrunkMesh->SetupAttachment(PhysicsRoot);
	TrunkMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -190.0f));
	TrunkMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 3.8f));
	TrunkMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CrownMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrownMesh"));
	CrownMesh->SetupAttachment(PhysicsRoot);
	CrownMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	CrownMesh->SetRelativeScale3D(FVector(2.2f, 2.2f, 2.7f));
	CrownMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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
	LogPickupClass = AChopItLogPickup::StaticClass();
}

void AChopItTree::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnHealthChanged.AddUObject(this, &AChopItTree::HandleHealthChanged);
	HealthComponent->OnDeath.AddUObject(this, &AChopItTree::HandleDepleted);
	UpdateHealthLabel(HealthComponent->GetCurrentHealth(), HealthComponent->GetMaxHealth());
	if (UWorld* World = GetWorld())
	{
		if (UChopItForestRegistrySubsystem* Registry = World->GetSubsystem<UChopItForestRegistrySubsystem>())
		{
			Registry->RegisterTree(this);
		}
	}
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
	CrownMesh->SetMaterial(0, CrownMaterial);
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
		0.2f,
		true,
		MinimumFallDuration);
	UE_LOG(LogChopIt, Display, TEXT("Tree %s entered Falling."), *GetName());
}

void AChopItTree::CheckFallSettled()
{
	if (HarvestState != EChopItTreeHarvestState::Falling)
	{
		GetWorldTimerManager().ClearTimer(FallSettleTimerHandle);
		return;
	}

	const double Elapsed = GetWorld()->GetTimeSeconds() - FallStartedAt;
	const bool bMotionSettled = PhysicsRoot->GetPhysicsLinearVelocity().SizeSquared() < FMath::Square(15.0)
		&& PhysicsRoot->GetPhysicsAngularVelocityInDegrees().SizeSquared() < FMath::Square(8.0);
	if (bMotionSettled || Elapsed >= MaximumFallDuration)
	{
		SettleAndSpawnReward();
	}
}

void AChopItTree::SettleAndSpawnReward()
{
	if (HarvestState != EChopItTreeHarvestState::Falling)
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(FallSettleTimerHandle);
	PhysicsRoot->SetSimulatePhysics(false);
	PhysicsRoot->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PhysicsRoot->SetCollisionResponseToAllChannels(ECR_Ignore);
	HarvestState = EChopItTreeHarvestState::Settled;
	SpawnRewardOnce();
}

void AChopItTree::SpawnRewardOnce()
{
	if (bRewardSpawned || HarvestState != EChopItTreeHarvestState::Settled)
	{
		return;
	}
	bRewardSpawned = true;

	if (LogPickupClass)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AChopItLogPickup* Pickup = GetWorld()->SpawnActor<AChopItLogPickup>(
			LogPickupClass,
			PhysicsRoot->GetComponentLocation() + FVector(0.0f, 0.0f, 60.0f),
			FRotator::ZeroRotator,
			SpawnParameters);
		if (Pickup)
		{
			Pickup->InitializeReward(WoodRewardUnits, ExperienceReward);
		}
	}

	HarvestState = EChopItTreeHarvestState::Harvested;
	OnHarvested.Broadcast(this, WoodRewardUnits, ExperienceReward);
	UE_LOG(LogChopIt, Display, TEXT("Tree %s emitted one reward: wood=%d xp=%d."), *GetName(), WoodRewardUnits, ExperienceReward);
	SetLifeSpan(8.0f);
}

void AChopItTree::UpdateHealthLabel(const float CurrentHealth, const float MaxHealth)
{
	HealthLabel->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), CurrentHealth, MaxHealth)));
}
