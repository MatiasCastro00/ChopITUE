#include "Harvest/ChopItLogPickup.h"

#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Harvest/ChopItForestRegistrySubsystem.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Progression/ChopItExperienceComponent.h"
#include "UObject/ConstructorHelpers.h"

AChopItLogPickup::AChopItLogPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	MagnetSphere = CreateDefaultSubobject<USphereComponent>(TEXT("MagnetSphere"));
	SetRootComponent(MagnetSphere);
	MagnetSphere->InitSphereRadius(450.0f);
	MagnetSphere->SetCollisionProfileName(ChopItCollisionProfiles::Pickup);
	MagnetSphere->SetGenerateOverlapEvents(true);

	LogMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LogMesh"));
	LogMesh->SetupAttachment(MagnetSphere);
	LogMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LogMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	LogMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.7f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		LogMesh->SetStaticMesh(CylinderMesh.Object);
	}

	UnitLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("UnitLabel"));
	UnitLabel->SetupAttachment(MagnetSphere);
	UnitLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));
	UnitLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	UnitLabel->SetHorizontalAlignment(EHTA_Center);
	UnitLabel->SetWorldSize(36.0f);
	UnitLabel->SetTextRenderColor(FColor::Yellow);
}

void AChopItLogPickup::BeginPlay()
{
	Super::BeginPlay();
	MagnetSphere->OnComponentBeginOverlap.AddDynamic(this, &AChopItLogPickup::HandleMagnetBeginOverlap);
	MagnetSphere->OnComponentEndOverlap.AddDynamic(this, &AChopItLogPickup::HandleMagnetEndOverlap);
	UpdateLabel();
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
	}
}

void AChopItLogPickup::UpdateMagnetism()
{
	UChopItWoodCargoComponent* Cargo = CandidateCargo.Get();
	AActor* CargoOwner = Cargo ? Cargo->GetOwner() : nullptr;
	if (!Cargo || !IsValid(CargoOwner))
	{
		GetWorldTimerManager().ClearTimer(MagnetTimerHandle);
		return;
	}
	if (Cargo->GetAvailableCapacity() <= 0)
	{
		return;
	}

	const FVector TargetLocation = CargoOwner->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
	const FVector NewLocation = FMath::VInterpConstantTo(GetActorLocation(), TargetLocation, 0.05f, MagnetSpeed);
	SetActorLocation(NewLocation, false);
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
			? FString::Printf(TEXT("Madera x%d  +%d XP"), WoodUnits, ExperienceReward)
			: FString::Printf(TEXT("Madera x%d"), WoodUnits)));
	}
}
