#include "Economy/ChopItDeliveryZone.h"

#include "ChopItCollision.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/ChopItCameraFacingTextComponent.h"
#include "Economy/ChopItQuotaComponent.h"
#include "Economy/ChopItQuotaMachine.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AChopItDeliveryZone::AChopItDeliveryZone()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	DeliverySphere = CreateDefaultSubobject<USphereComponent>(TEXT("DeliverySphere"));
	SetRootComponent(DeliverySphere);
	DeliverySphere->InitSphereRadius(230.0f);
	DeliverySphere->SetCollisionProfileName(ChopItCollisionProfiles::DeliveryZone);
	DeliverySphere->SetGenerateOverlapEvents(true);

	ZoneVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneVisual"));
	ZoneVisual->SetupAttachment(DeliverySphere);
	ZoneVisual->SetRelativeScale3D(FVector(2.3f, 2.3f, 0.055f));
	ZoneVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ZoneLabel = CreateDefaultSubobject<UChopItCameraFacingTextComponent>(TEXT("ZoneLabel"));
	ZoneLabel->SetupAttachment(DeliverySphere);
	ZoneLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));
	ZoneLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	ZoneLabel->SetHorizontalAlignment(EHTA_Center);
	ZoneLabel->SetWorldSize(34.0f);
	ZoneLabel->SetText(FText::FromString(TEXT("E  DELIVER WOOD")));
	ZoneLabel->SetTextRenderColor(FColor(255, 176, 32));
	ZoneLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LogPool = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PooledFlyingLogs"));
	LogPool->SetupAttachment(DeliverySphere);
	LogPool->SetMobility(EComponentMobility::Movable);
	LogPool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LogPool->SetGenerateOverlapEvents(false);
	LogPool->SetCastShadow(true);

	TrailPool = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PooledLogTrails"));
	TrailPool->SetupAttachment(DeliverySphere);
	TrailPool->SetMobility(EComponentMobility::Movable);
	TrailPool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TrailPool->SetGenerateOverlapEvents(false);
	TrailPool->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (CylinderMesh.Succeeded())
	{
		ZoneVisual->SetStaticMesh(CylinderMesh.Object);
		LogPool->SetStaticMesh(CylinderMesh.Object);
	}
	if (SphereMesh.Succeeded()) TrailPool->SetStaticMesh(SphereMesh.Object);
}

void AChopItDeliveryZone::BeginPlay()
{
	Super::BeginPlay();
	DeliverySphere->OnComponentBeginOverlap.AddDynamic(this, &AChopItDeliveryZone::HandleBeginOverlap);
	DeliverySphere->OnComponentEndOverlap.AddDynamic(this, &AChopItDeliveryZone::HandleEndOverlap);
	ZoneLabel->SetVisibility(false, true);
	ZoneLabel->SetHiddenInGame(true, true);

	if (UMaterialInterface* WoodMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/ChopIt/World/Blockout/Materials/MI_Wood.MI_Wood")))
	{
		ZoneVisual->SetMaterial(0, WoodMaterial);
		LogPool->SetMaterial(0, WoodMaterial);
	}
	if (UMaterialInterface* TrailMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/ChopIt/World/Blockout/Materials/MI_Player.MI_Player")))
	{
		TrailPool->SetMaterial(0, TrailMaterial);
	}

	Flights.SetNum(FMath::Max(8, PoolSize));
	VisualRandom.Initialize(GetUniqueID() * 196613 + 17);
	for (int32 Index = 0; Index < Flights.Num(); ++Index)
	{
		LogPool->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -100000.0f), FVector::ZeroVector));
		for (int32 Segment = 0; Segment < TrailSegmentsPerLog; ++Segment)
		{
			TrailPool->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -100000.0f), FVector::ZeroVector));
		}
	}
	ResolveTargetMachine();
}

void AChopItDeliveryZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(LaunchTimerHandle);
	Super::EndPlay(EndPlayReason);
}

bool AChopItDeliveryZone::CanInteract_Implementation(AActor* Interactor) const
{
	const UChopItWoodCargoComponent* Cargo = Interactor
		? Interactor->FindComponentByClass<UChopItWoodCargoComponent>() : nullptr;
	const UChopItQuotaComponent* Quota = ResolveQuota();
	return Cargo && Cargo == NearbyCargo.Get() && Cargo->GetCurrentWood() > 0
		&& Quota && !Quota->IsComplete() && !ActiveCargo.IsValid();
}

bool AChopItDeliveryZone::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor)) return false;
	ActiveCargo = Interactor->FindComponentByClass<UChopItWoodCargoComponent>();
	LaunchNextLog();
	if (ActiveCargo.IsValid())
	{
		GetWorldTimerManager().SetTimer(LaunchTimerHandle, this, &AChopItDeliveryZone::LaunchNextLog, LaunchInterval, true);
	}
	RefreshPrompt();
	RefreshTickState();
	return true;
}

void AChopItDeliveryZone::HandleBeginOverlap(
	UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (UChopItWoodCargoComponent* Cargo = OtherActor
		? OtherActor->FindComponentByClass<UChopItWoodCargoComponent>() : nullptr)
	{
		NearbyCargo = Cargo;
		ZoneLabel->SetVisibility(true, true);
		ZoneLabel->SetHiddenInGame(false, true);
		RefreshPrompt();
		RefreshTickState();
	}
}

void AChopItDeliveryZone::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
	if (NearbyCargo.IsValid() && NearbyCargo->GetOwner() == OtherActor)
	{
		NearbyCargo.Reset();
		ZoneLabel->SetVisibility(false, true);
		ZoneLabel->SetHiddenInGame(true, true);
		// The accepted delivery intentionally survives leaving the trigger.
		RefreshTickState();
	}
}

FVector AChopItDeliveryZone::EvaluateParabolicFlight(
	const FVector& Start,
	const FVector& Target,
	const FVector& LateralOffset,
	const float ArcHeight,
	const float Alpha)
{
	const float T = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const float Smoothed = FMath::SmoothStep(0.0f, 1.0f, T);
	return FMath::Lerp(Start, Target, Smoothed)
		+ FVector::UpVector * (4.0f * FMath::Max(0.0f, ArcHeight) * T * (1.0f - T))
		+ LateralOffset * FMath::Sin(PI * T);
}

void AChopItDeliveryZone::LaunchNextLog()
{
	UChopItWoodCargoComponent* Cargo = ActiveCargo.Get();
	UChopItQuotaComponent* Quota = ResolveQuota();
	if (!Cargo || !Quota || Cargo->GetCurrentWood() <= 0 || Quota->IsComplete()
		|| Quota->GetRemaining() <= PendingUnits)
	{
		StopLaunching();
		return;
	}

	const int32 PoolIndex = Flights.IndexOfByPredicate([](const FDeliveryFlight& Flight) { return !Flight.bActive; });
	if (PoolIndex == INDEX_NONE) return;
	if (Cargo->TryRemoveWood(1).Transferred != 1)
	{
		StopLaunching();
		return;
	}

	AChopItQuotaMachine* Machine = ResolveTargetMachine();
	const FVector OwnerLocation = Cargo->GetOwner() ? Cargo->GetOwner()->GetActorLocation() : GetActorLocation();
	FDeliveryFlight& Flight = Flights[PoolIndex];
	Flight.bActive = true;
	Flight.Start = OwnerLocation + FVector(
		VisualRandom.FRandRange(-22.0f, 22.0f),
		VisualRandom.FRandRange(-22.0f, 22.0f),
		VisualRandom.FRandRange(65.0f, 100.0f));
	Flight.Target = Machine ? Machine->GetDeliveryIntakeWorldLocation()
		: GetActorLocation() + FVector(0.0f, 0.0f, 280.0f);
	Flight.Target += FVector(VisualRandom.FRandRange(-35.0f, 35.0f), VisualRandom.FRandRange(-35.0f, 35.0f), 0.0f);
	const FVector Travel = Flight.Target - Flight.Start;
	const FVector Side = FVector::CrossProduct(Travel.GetSafeNormal(), FVector::UpVector).GetSafeNormal();
	Flight.LateralOffset = Side * VisualRandom.FRandRange(-90.0f, 90.0f);
	Flight.InitialRotation = FRotator(VisualRandom.FRandRange(-30.0f, 30.0f), VisualRandom.FRandRange(0.0f, 360.0f), 90.0f);
	Flight.RotationRate = FRotator(VisualRandom.FRandRange(240.0f, 520.0f), VisualRandom.FRandRange(-360.0f, 360.0f), VisualRandom.FRandRange(-240.0f, 240.0f));
	Flight.Elapsed = 0.0f;
	Flight.Duration = VisualRandom.FRandRange(MinimumFlightDuration, MaximumFlightDuration);
	Flight.ArcHeight = VisualRandom.FRandRange(260.0f, 430.0f);
	++PendingUnits;
	LogPool->UpdateInstanceTransform(PoolIndex,
		FTransform(Flight.InitialRotation, Flight.Start, FVector(0.10f, 0.10f, 0.34f)), true, true, true);
	RefreshTickState();
}

void AChopItDeliveryZone::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	VisualTime += DeltaSeconds;
	const float Pulse = 1.0f + FMath::Sin(VisualTime * 5.5f) * 0.035f;
	ZoneVisual->SetRelativeScale3D(FVector(2.3f * Pulse, 2.3f * Pulse, 0.055f));

	bool bAnyTransformChanged = false;
	for (int32 Index = 0; Index < Flights.Num(); ++Index)
	{
		FDeliveryFlight& Flight = Flights[Index];
		if (!Flight.bActive) continue;
		Flight.Elapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(Flight.Elapsed / FMath::Max(0.01f, Flight.Duration), 0.0f, 1.0f);
		if (Alpha >= 1.0f)
		{
			CompleteFlight(Index);
			continue;
		}
		const FVector Position = EvaluateParabolicFlight(Flight.Start, Flight.Target, Flight.LateralOffset, Flight.ArcHeight, Alpha);
		const FRotator Rotation = Flight.InitialRotation + Flight.RotationRate * Flight.Elapsed;
		const float ConsumeScale = Alpha > 0.82f
			? FMath::GetMappedRangeValueClamped(FVector2D(0.82f, 1.0f), FVector2D(1.0f, 0.12f), Alpha) : 1.0f;
		LogPool->UpdateInstanceTransform(Index,
			FTransform(Rotation, Position, FVector(0.10f, 0.10f, 0.34f) * ConsumeScale), true, false, true);
		UpdateTrailInstances(Index, Alpha);
		bAnyTransformChanged = true;
	}
	if (bAnyTransformChanged)
	{
		LogPool->MarkRenderStateDirty();
		TrailPool->MarkRenderStateDirty();
	}
	RefreshTickState();
}

void AChopItDeliveryZone::CompleteFlight(const int32 PoolIndex)
{
	if (!Flights.IsValidIndex(PoolIndex) || !Flights[PoolIndex].bActive) return;
	Flights[PoolIndex].bActive = false;
	PendingUnits = FMath::Max(0, PendingUnits - 1);
	HidePoolInstance(PoolIndex);
	HideTrailInstances(PoolIndex);

	if (UChopItQuotaComponent* Quota = ResolveQuota())
	{
		const FChopItQuotaTransferResult Result = Quota->TryContributeWood(FGuid::NewGuid(), 1);
		if (Result.Accepted > 0)
		{
			if (AChopItQuotaMachine* Machine = ResolveTargetMachine()) Machine->NotifyWoodConsumed(Result.Accepted);
		}
		else if (UChopItWoodCargoComponent* Cargo = ActiveCargo.Get())
		{
			Cargo->GrantWoodForTesting(1);
		}
	}
	RefreshPrompt();
}

void AChopItDeliveryZone::StopLaunching()
{
	GetWorldTimerManager().ClearTimer(LaunchTimerHandle);
	ActiveCargo.Reset();
	RefreshPrompt();
	RefreshTickState();
}

void AChopItDeliveryZone::RefreshPrompt()
{
	if (!NearbyCargo.IsValid()) return;
	const UChopItQuotaComponent* Quota = ResolveQuota();
	if (Quota && Quota->IsComplete())
	{
		ZoneLabel->SetText(FText::FromString(TEXT("QUOTA COMPLETE")));
		ZoneLabel->SetTextRenderColor(FColor::Green);
	}
	else if (ActiveCargo.IsValid())
	{
		ZoneLabel->SetText(FText::FromString(TEXT("DELIVERING...")));
		ZoneLabel->SetTextRenderColor(FColor(255, 210, 80));
	}
	else if (NearbyCargo->GetCurrentWood() <= 0)
	{
		ZoneLabel->SetText(FText::FromString(TEXT("NO WOOD")));
		ZoneLabel->SetTextRenderColor(FColor::Silver);
	}
	else
	{
		ZoneLabel->SetText(FText::FromString(TEXT("E  DELIVER WOOD")));
		ZoneLabel->SetTextRenderColor(FColor(255, 176, 32));
	}
}

void AChopItDeliveryZone::RefreshTickState()
{
	SetActorTickEnabled(NearbyCargo.IsValid() || GetActiveFlightCount() > 0);
}

void AChopItDeliveryZone::HidePoolInstance(const int32 PoolIndex)
{
	LogPool->UpdateInstanceTransform(PoolIndex,
		FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -100000.0f), FVector::ZeroVector), false, true, true);
}

void AChopItDeliveryZone::HideTrailInstances(const int32 PoolIndex)
{
	const int32 FirstTrailIndex = PoolIndex * TrailSegmentsPerLog;
	for (int32 Segment = 0; Segment < TrailSegmentsPerLog; ++Segment)
	{
		TrailPool->UpdateInstanceTransform(FirstTrailIndex + Segment,
			FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -100000.0f), FVector::ZeroVector), false, false, true);
	}
	TrailPool->MarkRenderStateDirty();
}

void AChopItDeliveryZone::UpdateTrailInstances(const int32 PoolIndex, const float FlightAlpha)
{
	if (!Flights.IsValidIndex(PoolIndex)) return;
	const FDeliveryFlight& Flight = Flights[PoolIndex];
	const int32 FirstTrailIndex = PoolIndex * TrailSegmentsPerLog;
	for (int32 Segment = 0; Segment < TrailSegmentsPerLog; ++Segment)
	{
		const float Delay = 0.026f * static_cast<float>(Segment + 1);
		const float SampleAlpha = FlightAlpha - Delay;
		const int32 InstanceIndex = FirstTrailIndex + Segment;
		if (SampleAlpha <= 0.0f)
		{
			TrailPool->UpdateInstanceTransform(InstanceIndex,
				FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -100000.0f), FVector::ZeroVector), false, false, true);
			continue;
		}
		const FVector TrailPosition = EvaluateParabolicFlight(
			Flight.Start, Flight.Target, Flight.LateralOffset, Flight.ArcHeight, SampleAlpha);
		const float Fade = 1.0f - static_cast<float>(Segment) / FMath::Max(1.0f, static_cast<float>(TrailSegmentsPerLog));
		const float Radius = 0.050f * Fade * FMath::Min(1.0f, FlightAlpha * 12.0f);
		TrailPool->UpdateInstanceTransform(InstanceIndex,
			FTransform(FRotator::ZeroRotator, TrailPosition, FVector(Radius)), true, false, true);
	}
}

int32 AChopItDeliveryZone::GetActiveFlightCount() const
{
	int32 ActiveCount = 0;
	for (const FDeliveryFlight& Flight : Flights) ActiveCount += Flight.bActive ? 1 : 0;
	return ActiveCount;
}

UChopItQuotaComponent* AChopItDeliveryZone::ResolveQuota() const
{
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return GameState ? GameState->FindComponentByClass<UChopItQuotaComponent>() : nullptr;
}

AChopItQuotaMachine* AChopItDeliveryZone::ResolveTargetMachine()
{
	if (TargetMachine.IsValid()) return TargetMachine.Get();
	if (!GetWorld()) return nullptr;
	for (TActorIterator<AChopItQuotaMachine> It(GetWorld()); It; ++It)
	{
		TargetMachine = *It;
		break;
	}
	return TargetMachine.Get();
}
