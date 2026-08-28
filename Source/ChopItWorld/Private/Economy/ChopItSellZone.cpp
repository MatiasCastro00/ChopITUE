#include "Economy/ChopItSellZone.h"

#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Economy/ChopItEconomyComponent.h"
#include "Economy/ChopItQuotaComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "UObject/ConstructorHelpers.h"

AChopItSellZone::AChopItSellZone()
{
	PrimaryActorTick.bCanEverTick = false;
	SellSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SellSphere"));
	SetRootComponent(SellSphere);
	SellSphere->InitSphereRadius(230.0f);
	SellSphere->SetCollisionProfileName(ChopItCollisionProfiles::DeliveryZone);
	SellSphere->SetGenerateOverlapEvents(true);

	ZoneVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneVisual"));
	ZoneVisual->SetupAttachment(SellSphere);
	ZoneVisual->SetRelativeScale3D(FVector(2.3f, 2.3f, 0.08f));
	ZoneVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ZoneLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ZoneLabel"));
	ZoneLabel->SetupAttachment(SellSphere);
	ZoneLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	ZoneLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	ZoneLabel->SetHorizontalAlignment(EHTA_Center);
	ZoneLabel->SetWorldSize(32.0f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		ZoneVisual->SetStaticMesh(CylinderMesh.Object);
	}
}

void AChopItSellZone::BeginPlay()
{
	Super::BeginPlay();
	SellSphere->OnComponentBeginOverlap.AddDynamic(this, &AChopItSellZone::HandleBeginOverlap);
	SellSphere->OnComponentEndOverlap.AddDynamic(this, &AChopItSellZone::HandleEndOverlap);
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	UChopItQuotaComponent* Quota = GameState ? GameState->FindComponentByClass<UChopItQuotaComponent>() : nullptr;
	if (Quota)
	{
		Quota->OnQuotaChanged.AddUniqueDynamic(this, &AChopItSellZone::HandleQuotaChanged);
		HandleQuotaChanged(Quota->GetProgress(), Quota->GetTarget(), Quota->IsComplete());
	}
}

void AChopItSellZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(TransferTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AChopItSellZone::HandleBeginOverlap(
	UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	UChopItWoodCargoComponent* Cargo = OtherActor ? OtherActor->FindComponentByClass<UChopItWoodCargoComponent>() : nullptr;
	if (Cargo)
	{
		CandidateCargo = Cargo;
		SellBatch();
		GetWorldTimerManager().SetTimer(TransferTimerHandle, this, &AChopItSellZone::SellBatch, TransferInterval, true);
	}
}

void AChopItSellZone::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
	if (CandidateCargo.IsValid() && CandidateCargo->GetOwner() == OtherActor)
	{
		CandidateCargo.Reset();
		GetWorldTimerManager().ClearTimer(TransferTimerHandle);
	}
}

void AChopItSellZone::HandleQuotaChanged(const int32, const int32, const bool bComplete)
{
	ZoneLabel->SetText(FText::FromString(bComplete
		? FString::Printf(TEXT("VENDER MADERA  $%lld"), MoneyPerWood)
		: TEXT("CAMIONETA BLOQUEADA")));
	ZoneLabel->SetTextRenderColor(bComplete ? FColor::Green : FColor::Silver);
}

bool AChopItSellZone::CanSell(
	const UChopItQuotaComponent* Quota,
	const UChopItWoodCargoComponent* Cargo,
	const UChopItEconomyComponent* Economy)
{
	return Quota && Quota->IsComplete() && Cargo && Cargo->GetCurrentWood() > 0 && Economy;
}

void AChopItSellZone::SellBatch()
{
	UChopItWoodCargoComponent* Cargo = CandidateCargo.Get();
	APawn* Pawn = Cargo ? Cast<APawn>(Cargo->GetOwner()) : nullptr;
	APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState() : nullptr;
	UChopItEconomyComponent* Economy = PlayerState ? PlayerState->FindComponentByClass<UChopItEconomyComponent>() : nullptr;
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	UChopItQuotaComponent* Quota = GameState ? GameState->FindComponentByClass<UChopItQuotaComponent>() : nullptr;
	if (!CanSell(Quota, Cargo, Economy))
	{
		return;
	}

	const int32 Units = FMath::Min(BatchSize, Cargo->GetCurrentWood());
	if (MoneyPerWood <= 0 || Units > MAX_int64 / MoneyPerWood)
	{
		return;
	}
	const FGuid TransactionId = FGuid::NewGuid();
	const int64 Revenue = static_cast<int64>(Units) * MoneyPerWood;
	if (Economy->ApplyTransaction(TransactionId, TEXT("WoodSale"), Revenue))
	{
		const FChopItWoodTransferResult CargoResult = Cargo->TryRemoveWood(Units);
		check(CargoResult.Transferred == Units);
		UE_LOG(LogChopIt, Display, TEXT("Wood sale: units=%d revenue=%lld balance=%lld transaction=%s"),
			Units, Revenue, Economy->GetBalance(), *TransactionId.ToString());
	}
}
