#include "Economy/ChopItDeliveryZone.h"

#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Economy/ChopItQuotaComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "UObject/ConstructorHelpers.h"

AChopItDeliveryZone::AChopItDeliveryZone()
{
	PrimaryActorTick.bCanEverTick = false;
	DeliverySphere = CreateDefaultSubobject<USphereComponent>(TEXT("DeliverySphere"));
	SetRootComponent(DeliverySphere);
	DeliverySphere->InitSphereRadius(230.0f);
	DeliverySphere->SetCollisionProfileName(ChopItCollisionProfiles::DeliveryZone);
	DeliverySphere->SetGenerateOverlapEvents(true);

	ZoneVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneVisual"));
	ZoneVisual->SetupAttachment(DeliverySphere);
	ZoneVisual->SetRelativeScale3D(FVector(2.3f, 2.3f, 0.08f));
	ZoneVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ZoneLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ZoneLabel"));
	ZoneLabel->SetupAttachment(DeliverySphere);
	ZoneLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	ZoneLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	ZoneLabel->SetHorizontalAlignment(EHTA_Center);
	ZoneLabel->SetWorldSize(32.0f);
	ZoneLabel->SetText(FText::FromString(TEXT("ENTREGAR CUOTA")));
	ZoneLabel->SetTextRenderColor(FColor::Red);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		ZoneVisual->SetStaticMesh(CylinderMesh.Object);
	}
}

void AChopItDeliveryZone::BeginPlay()
{
	Super::BeginPlay();
	DeliverySphere->OnComponentBeginOverlap.AddDynamic(this, &AChopItDeliveryZone::HandleBeginOverlap);
	DeliverySphere->OnComponentEndOverlap.AddDynamic(this, &AChopItDeliveryZone::HandleEndOverlap);
}

void AChopItDeliveryZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(TransferTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AChopItDeliveryZone::HandleBeginOverlap(
	UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	UChopItWoodCargoComponent* Cargo = OtherActor ? OtherActor->FindComponentByClass<UChopItWoodCargoComponent>() : nullptr;
	if (Cargo)
	{
		CandidateCargo = Cargo;
		TransferQuotaBatch();
		GetWorldTimerManager().SetTimer(TransferTimerHandle, this, &AChopItDeliveryZone::TransferQuotaBatch, TransferInterval, true);
	}
}

void AChopItDeliveryZone::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
	if (CandidateCargo.IsValid() && CandidateCargo->GetOwner() == OtherActor)
	{
		CandidateCargo.Reset();
		GetWorldTimerManager().ClearTimer(TransferTimerHandle);
	}
}

void AChopItDeliveryZone::TransferQuotaBatch()
{
	UChopItWoodCargoComponent* Cargo = CandidateCargo.Get();
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	UChopItQuotaComponent* Quota = GameState ? GameState->FindComponentByClass<UChopItQuotaComponent>() : nullptr;
	if (!Cargo || !Quota || Cargo->GetCurrentWood() <= 0 || Quota->IsComplete())
	{
		return;
	}

	const int32 Requested = FMath::Min3(BatchSize, Cargo->GetCurrentWood(), Quota->GetRemaining());
	const FGuid TransactionId = FGuid::NewGuid();
	const FChopItQuotaTransferResult QuotaResult = Quota->TryContributeWood(TransactionId, Requested);
	if (QuotaResult.Accepted > 0)
	{
		const FChopItWoodTransferResult CargoResult = Cargo->TryRemoveWood(QuotaResult.Accepted);
		check(CargoResult.Transferred == QuotaResult.Accepted);
		UE_LOG(LogChopIt, Display, TEXT("Quota delivery: accepted=%d progress=%d/%d transaction=%s"),
			QuotaResult.Accepted, Quota->GetProgress(), Quota->GetTarget(), *TransactionId.ToString());
	}
}
