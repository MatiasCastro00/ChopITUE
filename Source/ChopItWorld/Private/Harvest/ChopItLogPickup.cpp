#include "Harvest/ChopItLogPickup.h"

#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/ChopItCameraFacingTextComponent.h"
#include "Engine/World.h"
#include "Harvest/ChopItForestRegistrySubsystem.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Progression/ChopItExperienceComponent.h"
#include "UObject/ConstructorHelpers.h"

AChopItLogPickup::AChopItLogPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	PhysicsBody = CreateDefaultSubobject<USphereComponent>(TEXT("PhysicsBody"));
	SetRootComponent(PhysicsBody);
	PhysicsBody->InitSphereRadius(28.0f);
	PhysicsBody->SetMobility(EComponentMobility::Movable);
	PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsBody->SetCollisionObjectType(ChopItCollisionChannels::Pickup);
	PhysicsBody->SetCollisionResponseToAllChannels(ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	PhysicsBody->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	PhysicsBody->SetEnableGravity(true);
	PhysicsBody->SetSimulatePhysics(true);
	PhysicsBody->SetUseCCD(true);
	PhysicsBody->SetLinearDamping(0.65f);
	PhysicsBody->SetAngularDamping(0.8f);

	LogMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LogMesh"));
	LogMesh->SetupAttachment(PhysicsBody);
	LogMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	LogMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.7f));
	LogMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MagnetSphere = CreateDefaultSubobject<USphereComponent>(TEXT("MagnetSphere"));
	MagnetSphere->SetupAttachment(PhysicsBody);
	MagnetSphere->InitSphereRadius(450.0f);
	MagnetSphere->SetCollisionProfileName(ChopItCollisionProfiles::Pickup);
	MagnetSphere->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		LogMesh->SetStaticMesh(CylinderMesh.Object);
	}

	UnitLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("UnitLabel"));
	UnitLabel->SetupAttachment(PhysicsBody);
	UnitLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));
	UnitLabel->SetUsingAbsoluteRotation(true);
	UnitLabel->SetHorizontalAlignment(EHTA_Center);
	UnitLabel->SetWorldSize(36.0f);
	UnitLabel->SetTextRenderColor(FColor::Yellow);
	UnitLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UChopItCameraFacingTextComponent::ConfigurePsxBypass(*UnitLabel);
}

void AChopItLogPickup::BeginPlay()
{
	Super::BeginPlay();
	// Existing Blueprint children may retain their old no-collision component
	// settings, so enforce the physical drop profile at runtime as well.
	ConfigureDroppedPhysics();
	PreviousPhysicsLocation = PhysicsBody->GetComponentLocation();
	MagnetSphere->OnComponentBeginOverlap.AddDynamic(this, &AChopItLogPickup::HandleMagnetBeginOverlap);
	MagnetSphere->OnComponentEndOverlap.AddDynamic(this, &AChopItLogPickup::HandleMagnetEndOverlap);
	GetWorldTimerManager().SetTimer(
		GroundSafetyTimerHandle,
		this,
		&AChopItLogPickup::UpdateGroundSafety,
		1.0f / 60.0f,
		true);
	UpdateLabel();
	UpdateLabelFacingCamera();
	if (UWorld* World = GetWorld())
	{
		if (UChopItForestRegistrySubsystem* Registry = World->GetSubsystem<UChopItForestRegistrySubsystem>())
		{
			Registry->RegisterLogPickup(this);
		}
	}
}

void AChopItLogPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MagnetTimerHandle);
		World->GetTimerManager().ClearTimer(GroundSafetyTimerHandle);
		if (UChopItForestRegistrySubsystem* Registry = World->GetSubsystem<UChopItForestRegistrySubsystem>())
		{
			Registry->UnregisterLogPickup(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AChopItLogPickup::InitializeWoodUnits(const int32 NewWoodUnits)
{
	InitializeReward(NewWoodUnits, 0);
}

void AChopItLogPickup::InitializeReward(const int32 NewWoodUnits, const int32 NewExperienceReward)
{
	WoodUnits = FMath::Max(1, NewWoodUnits);
	ExperienceReward = FMath::Max(0, NewExperienceReward);
	UpdateLabel();
}

void AChopItLogPickup::LaunchFromImpact(
	const FVector& LinearVelocity,
	const FVector& AngularVelocityDegrees)
{
	CandidateCargo.Reset();
	GetWorldTimerManager().ClearTimer(MagnetTimerHandle);
	ConfigureDroppedPhysics();
	PhysicsBody->SetPhysicsLinearVelocity(LinearVelocity, false);
	PhysicsBody->SetPhysicsAngularVelocityInDegrees(AngularVelocityDegrees, false);
	PhysicsBody->WakeAllRigidBodies();
}

void AChopItLogPickup::HandleMagnetBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	UChopItWoodCargoComponent* Cargo = OtherActor ? OtherActor->FindComponentByClass<UChopItWoodCargoComponent>() : nullptr;
	if (!Cargo)
	{
		return;
	}
	// The mesh is the physics root while dropped. Disable that body before the
	// gameplay magnet starts moving the actor, avoiding forces against the player.
	PhysicsBody->SetSimulatePhysics(false);
	PhysicsBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviousPhysicsLocation = GetActorLocation();
	CandidateCargo = Cargo;
	GetWorldTimerManager().SetTimer(MagnetTimerHandle, this, &AChopItLogPickup::UpdateMagnetism, 0.05f, true);
}

void AChopItLogPickup::HandleMagnetEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	if (CandidateCargo.IsValid() && CandidateCargo->GetOwner() == OtherActor)
	{
		CandidateCargo.Reset();
		GetWorldTimerManager().ClearTimer(MagnetTimerHandle);
		ResumeDroppedPhysics();
	}
}

void AChopItLogPickup::UpdateMagnetism()
{
	UChopItWoodCargoComponent* Cargo = CandidateCargo.Get();
	AActor* CargoOwner = Cargo ? Cargo->GetOwner() : nullptr;
	if (!Cargo || !IsValid(CargoOwner))
	{
		CandidateCargo.Reset();
		GetWorldTimerManager().ClearTimer(MagnetTimerHandle);
		ResumeDroppedPhysics();
		return;
	}
	if (Cargo->GetAvailableCapacity() <= 0)
	{
		CandidateCargo.Reset();
		GetWorldTimerManager().ClearTimer(MagnetTimerHandle);
		ResumeDroppedPhysics();
		return;
	}

	const FVector TargetLocation = CargoOwner->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
	const FVector NewLocation = FMath::VInterpConstantTo(GetActorLocation(), TargetLocation, 0.05f, MagnetSpeed);
	SetActorLocation(NewLocation, false);
	PreviousPhysicsLocation = NewLocation;
	if (FVector::DistSquared(NewLocation, TargetLocation) <= FMath::Square(CollectionDistance))
	{
		const FChopItWoodTransferResult Transfer = Cargo->TryAddWood(WoodUnits);
		WoodUnits = Transfer.Remainder;
		UE_LOG(LogChopIt, Display, TEXT("Log pickup transferred %d wood; %d remains."), Transfer.Transferred, WoodUnits);
		if (WoodUnits <= 0)
		{
			if (ExperienceReward > 0)
			{
				if (const APawn* Pawn = Cast<APawn>(CargoOwner))
				{
					if (APlayerState* PlayerState = Pawn->GetPlayerState())
					{
						if (UChopItExperienceComponent* Experience = PlayerState->FindComponentByClass<UChopItExperienceComponent>())
						{
							Experience->AddExperience(ExperienceReward);
						}
					}
				}
			}
			Destroy();
			return;
		}
		UpdateLabel();
	}
}

void AChopItLogPickup::UpdateLabel()
{
	if (UnitLabel)
	{
		UnitLabel->SetText(FText::FromString(ExperienceReward > 0
			? FString::Printf(TEXT("Wood x%d  +%d XP"), WoodUnits, ExperienceReward)
			: FString::Printf(TEXT("Wood x%d"), WoodUnits)));
	}
}

void AChopItLogPickup::UpdateLabelFacingCamera()
{
	if (!UnitLabel)
	{
		return;
	}

	const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!CameraManager)
	{
		return;
	}

	FVector ToCamera = CameraManager->GetCameraLocation() - UnitLabel->GetComponentLocation();
	if (!ToCamera.Normalize())
	{
		return;
	}
	UnitLabel->SetWorldRotation(FRotationMatrix::MakeFromXZ(ToCamera, FVector::UpVector).Rotator());
}

void AChopItLogPickup::ResumeDroppedPhysics()
{
	if (!PhysicsBody)
	{
		return;
	}
	PreviousPhysicsLocation = GetActorLocation();
	ConfigureDroppedPhysics();
	PhysicsBody->WakeAllRigidBodies();
}

void AChopItLogPickup::UpdateGroundSafety()
{
	UWorld* World = GetWorld();
	if (!World || !PhysicsBody)
	{
		return;
	}
	UpdateLabelFacingCamera();

	FVector CurrentLocation = PhysicsBody->GetComponentLocation();
	if (!PhysicsBody->IsSimulatingPhysics())
	{
		PreviousPhysicsLocation = CurrentLocation;
		return;
	}
	if (FVector::DistSquared(PreviousPhysicsLocation, CurrentLocation) < 0.01f)
	{
		PreviousPhysicsLocation = CurrentLocation;
		return;
	}

	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams(FName(TEXT("LogPickupGroundSafety")), false, this);
	QueryParams.bFindInitialOverlaps = true;
	FHitResult Hit;
	const bool bBlocked = World->SweepSingleByObjectType(
		Hit,
		PreviousPhysicsLocation,
		CurrentLocation,
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(26.0f),
		QueryParams);
	if (bBlocked && Hit.bBlockingHit)
	{
		const FVector SurfaceNormal = Hit.ImpactNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		const FVector CorrectedLocation = Hit.bStartPenetrating
			? CurrentLocation + SurfaceNormal * (Hit.PenetrationDepth + 2.0f)
			: Hit.Location + SurfaceNormal * 2.0f;
		SetActorLocation(CorrectedLocation, false, nullptr, ETeleportType::TeleportPhysics);

		FVector LinearVelocity = PhysicsBody->GetPhysicsLinearVelocity();
		const float InwardSpeed = FVector::DotProduct(LinearVelocity, SurfaceNormal);
		if (InwardSpeed < 0.0f)
		{
			LinearVelocity -= SurfaceNormal * InwardSpeed;
		}
		PhysicsBody->SetPhysicsLinearVelocity(LinearVelocity * 0.65f, false);
		PhysicsBody->SetPhysicsAngularVelocityInDegrees(
			PhysicsBody->GetPhysicsAngularVelocityInDegrees() * 0.8f,
			false);
		CurrentLocation = CorrectedLocation;
	}

	PreviousPhysicsLocation = CurrentLocation;
}

void AChopItLogPickup::ConfigureDroppedPhysics()
{
	if (!PhysicsBody)
	{
		return;
	}
	PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsBody->SetCollisionObjectType(ChopItCollisionChannels::Pickup);
	PhysicsBody->SetCollisionResponseToAllChannels(ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	PhysicsBody->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	PhysicsBody->SetEnableGravity(true);
	PhysicsBody->SetUseCCD(true);
	PhysicsBody->SetSimulatePhysics(true);
	PhysicsBody->SetMassOverrideInKg(NAME_None, 4.0f, true);
}
