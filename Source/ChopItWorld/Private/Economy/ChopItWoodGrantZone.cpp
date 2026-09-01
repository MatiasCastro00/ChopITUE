#include "Economy/ChopItWoodGrantZone.h"

#include "ChopItCollision.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/ChopItCameraFacingTextComponent.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AChopItWoodGrantZone::AChopItWoodGrantZone()
{
	PrimaryActorTick.bCanEverTick = false;
	GrantSphere = CreateDefaultSubobject<USphereComponent>(TEXT("GrantSphere"));
	SetRootComponent(GrantSphere);
	GrantSphere->InitSphereRadius(180.0f);
	GrantSphere->SetCollisionProfileName(ChopItCollisionProfiles::DeliveryZone);
	GrantSphere->SetGenerateOverlapEvents(true);

	ZoneVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneVisual"));
	ZoneVisual->SetupAttachment(GrantSphere);
	ZoneVisual->SetRelativeScale3D(FVector(1.8f, 1.8f, 0.045f));
	ZoneVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ZoneLabel = CreateDefaultSubobject<UChopItCameraFacingTextComponent>(TEXT("ZoneLabel"));
	ZoneLabel->SetupAttachment(GrantSphere);
	ZoneLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
	ZoneLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	ZoneLabel->SetHorizontalAlignment(EHTA_Center);
	ZoneLabel->SetWorldSize(31.0f);
	ZoneLabel->SetTextRenderColor(FColor::Cyan);
	ZoneLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded()) ZoneVisual->SetStaticMesh(CylinderMesh.Object);
}

void AChopItWoodGrantZone::BeginPlay()
{
	Super::BeginPlay();
	GrantSphere->OnComponentBeginOverlap.AddDynamic(this, &AChopItWoodGrantZone::HandleBeginOverlap);
	GrantSphere->OnComponentEndOverlap.AddDynamic(this, &AChopItWoodGrantZone::HandleEndOverlap);
	ZoneLabel->SetVisibility(false, true);
	ZoneLabel->SetHiddenInGame(true, true);
	if (UMaterialInterface* PlayerMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/ChopIt/World/Blockout/Materials/MI_Player.MI_Player")))
	{
		ZoneVisual->SetMaterial(0, PlayerMaterial);
	}
}

bool AChopItWoodGrantZone::CanInteract_Implementation(AActor* Interactor) const
{
	const UChopItWoodCargoComponent* Cargo = Interactor
		? Interactor->FindComponentByClass<UChopItWoodCargoComponent>() : nullptr;
	return Cargo && Cargo == NearbyCargo.Get() && Cargo->GetCurrentWood() < TargetWood;
}

bool AChopItWoodGrantZone::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor)) return false;
	UChopItWoodCargoComponent* Cargo = Interactor->FindComponentByClass<UChopItWoodCargoComponent>();
	const int32 Needed = FMath::Max(0, TargetWood - Cargo->GetCurrentWood());
	const bool bGranted = Cargo->GrantWoodForTesting(Needed).Transferred == Needed;
	RefreshPrompt();
	return bGranted;
}

void AChopItWoodGrantZone::HandleBeginOverlap(
	UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (UChopItWoodCargoComponent* Cargo = OtherActor
		? OtherActor->FindComponentByClass<UChopItWoodCargoComponent>() : nullptr)
	{
		NearbyCargo = Cargo;
		ZoneLabel->SetVisibility(true, true);
		ZoneLabel->SetHiddenInGame(false, true);
		RefreshPrompt();
	}
}

void AChopItWoodGrantZone::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
	if (NearbyCargo.IsValid() && NearbyCargo->GetOwner() == OtherActor)
	{
		NearbyCargo.Reset();
		ZoneLabel->SetVisibility(false, true);
		ZoneLabel->SetHiddenInGame(true, true);
	}
}

void AChopItWoodGrantZone::RefreshPrompt()
{
	if (!NearbyCargo.IsValid()) return;
	if (NearbyCargo->GetCurrentWood() >= TargetWood)
	{
		ZoneLabel->SetText(FText::FromString(TEXT("TEST READY: 200 LOGS")));
		ZoneLabel->SetTextRenderColor(FColor::Green);
	}
	else
	{
		ZoneLabel->SetText(FText::FromString(TEXT("E  GET 200 LOGS  [TEST]")));
		ZoneLabel->SetTextRenderColor(FColor::Cyan);
	}
}
