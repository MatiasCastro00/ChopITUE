#include "Economy/ChopItQuotaMachine.h"

#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Economy/ChopItChainDefinition.h"
#include "Economy/ChopItQuotaComponent.h"
#include "Economy/ChopItRopeComponent.h"
#include "Economy/ChopItTetherPathComponent.h"
#include "Economy/ChopItTetherReceiverComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AChopItQuotaMachine::AChopItQuotaMachine()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MachineVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MachineVisual"));
	MachineVisual->SetupAttachment(SceneRoot);
	MachineVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	MachineVisual->SetRelativeScale3D(FVector(1.2f, 1.2f, 2.0f));
	MachineVisual->SetCollisionProfileName(TEXT("BlockAll"));
	MachineVisual->SetCollisionResponseToChannel(ChopItCollisionChannels::Chain, ECR_Ignore);

	QuotaLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("QuotaLabel"));
	QuotaLabel->SetupAttachment(SceneRoot);
	QuotaLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 245.0f));
	QuotaLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	QuotaLabel->SetHorizontalAlignment(EHTA_Center);
	QuotaLabel->SetWorldSize(42.0f);
	QuotaLabel->SetTextRenderColor(FColor::Red);

	LeverLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LeverLabel"));
	LeverLabel->SetupAttachment(SceneRoot);
	LeverLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 295.0f));
	LeverLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	LeverLabel->SetHorizontalAlignment(EHTA_Center);
	LeverLabel->SetWorldSize(30.0f);
	LeverLabel->SetTextRenderColor(FColor::Silver);

	TetherPath = CreateDefaultSubobject<UChopItTetherPathComponent>(TEXT("AuthoritativeTetherPath"));
	TetherPath->SetupAttachment(SceneRoot);
	RopeSimulation = CreateDefaultSubobject<UChopItRopeComponent>(TEXT("VisualRopeSimulation"));
	RopeSimulation->SetupAttachment(SceneRoot);

	ChainLinkVisuals = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ChainLinkVisuals"));
	ChainLinkVisuals->SetupAttachment(SceneRoot);
	ChainLinkVisuals->SetMobility(EComponentMobility::Movable);
	ChainLinkVisuals->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChainLinkVisuals->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CubeMesh.Succeeded())
	{
		MachineVisual->SetStaticMesh(CubeMesh.Object);
	}
	if (CylinderMesh.Succeeded())
	{
		ChainLinkVisuals->SetStaticMesh(CylinderMesh.Object);
	}
}

void AChopItQuotaMachine::BeginPlay()
{
	Super::BeginPlay();
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain)
	{
		UE_LOG(LogChopIt, Error, TEXT("%s has no Chain Definition; player chain is disabled."), *GetName());
		return;
	}
	TetherPath->Configure(Chain);
	RopeSimulation->Configure(Chain);
	if (Chain->ChainLinkMesh)
	{
		ChainLinkVisuals->SetStaticMesh(Chain->ChainLinkMesh);
	}

	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (UChopItQuotaComponent* Quota = GameState ? GameState->FindComponentByClass<UChopItQuotaComponent>() : nullptr)
	{
		Quota->OnQuotaChanged.AddUniqueDynamic(this, &AChopItQuotaMachine::HandleQuotaChanged);
		HandleQuotaChanged(Quota->GetProgress(), Quota->GetTarget(), Quota->IsComplete());
	}
	if (UChopItCycleStateMachineComponent* Cycle = GameState
		? GameState->FindComponentByClass<UChopItCycleStateMachineComponent>() : nullptr)
	{
		Cycle->OnPhaseChanged.AddUniqueDynamic(this, &AChopItQuotaMachine::HandlePhaseChanged);
		Cycle->OnClockChanged.AddUniqueDynamic(this, &AChopItQuotaMachine::HandleClockChanged);
	}
	RefreshLeverLabel();
	TryCreatePlayerChain();
}

void AChopItQuotaMachine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChainCreationTimer);
	}
	DestroyPlayerChain();
	Super::EndPlay(EndPlayReason);
}

void AChopItQuotaMachine::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (const UChopItChainDefinition* Chain = GetChainDefinition(); Chain && Chain->bChainPlayerToMachine)
	{
		UpdateRetractableChain(DeltaSeconds);
	}
}

bool AChopItQuotaMachine::CanInteract_Implementation(AActor* Interactor) const
{
	return IsValid(Interactor);
}

bool AChopItQuotaMachine::Interact_Implementation(AActor* Interactor)
{
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	UChopItCycleStateMachineComponent* Cycle = GameState
		? GameState->FindComponentByClass<UChopItCycleStateMachineComponent>() : nullptr;
	const bool bAccepted = Cycle && Cycle->RequestLever(Interactor);
	RefreshLeverLabel();
	return bAccepted;
}

void AChopItQuotaMachine::HandleQuotaChanged(const int32 Progress, const int32 Target, const bool bComplete)
{
	QuotaLabel->SetText(FText::FromString(bComplete
		? FString::Printf(TEXT("CUOTA PAGADA  %d / %d"), Progress, Target)
		: FString::Printf(TEXT("CUOTA  %d / %d"), Progress, Target)));
	QuotaLabel->SetTextRenderColor(bComplete ? FColor::Green : FColor::Red);
	RefreshLeverLabel();
}

void AChopItQuotaMachine::HandlePhaseChanged(const EChopItCyclePhase, const EChopItCyclePhase, const int32)
{
	RefreshLeverLabel();
}

void AChopItQuotaMachine::HandleClockChanged(const EChopItCyclePhase, const float)
{
	RefreshLeverLabel();
}

void AChopItQuotaMachine::RefreshLeverLabel()
{
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const UChopItCycleStateMachineComponent* Cycle = GameState
		? GameState->FindComponentByClass<UChopItCycleStateMachineComponent>() : nullptr;
	const UChopItQuotaComponent* Quota = GameState
		? GameState->FindComponentByClass<UChopItQuotaComponent>() : nullptr;
	const bool bAvailable = Cycle && UChopItCycleStateMachineComponent::CanAcceptLever(
		Cycle->GetCurrentPhase(), Quota && Quota->IsComplete(), Cycle->IsNightMinimumElapsed());
	if (bAvailable)
	{
		LeverLabel->SetText(FText::FromString(TEXT("E: TIRAR PALANCA")));
		LeverLabel->SetTextRenderColor(FColor::Green);
	}
	else if (Cycle && Cycle->GetCurrentPhase() == EChopItCyclePhase::Night)
	{
		LeverLabel->SetText(FText::FromString(TEXT("PAGA LA CUOTA PARA LA PALANCA")));
		LeverLabel->SetTextRenderColor(FColor::Yellow);
	}
	else
	{
		LeverLabel->SetText(FText::FromString(TEXT("PALANCA BLOQUEADA")));
		LeverLabel->SetTextRenderColor(FColor::Silver);
	}
}

void AChopItQuotaMachine::TryCreatePlayerChain()
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain || !Chain->bChainPlayerToMachine || IsValid(ChainedPlayer))
	{
		return;
	}
	UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!IsValid(Pawn) || !Pawn->GetRootComponent())
	{
		if (World)
		{
			World->GetTimerManager().SetTimer(ChainCreationTimer, this, &AChopItQuotaMachine::TryCreatePlayerChain, 0.2f, false);
		}
		return;
	}
	CreatePlayerChain(Pawn);
}

void AChopItQuotaMachine::CreatePlayerChain(AActor* PlayerActor)
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain || !IsValid(PlayerActor) || !RopeSimulation || !TetherPath || !ChainLinkVisuals)
	{
		return;
	}
	DestroyPlayerChain();
	ChainedPlayer = PlayerActor;
	TetherReceiver = PlayerActor->FindComponentByClass<UChopItTetherReceiverComponent>();
	const FVector MachineAnchor = GetActorTransform().TransformPosition(Chain->MachineChainAnchor);
	const FVector PlayerAnchor = PlayerActor->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);
	TetherPath->InitializePath(MachineAnchor, PlayerAnchor, PlayerActor);
	const float MinimumLength = FMath::Clamp(Chain->MinimumDeployedLinks, 3, Chain->ChainLinkCount) * GetFixedLinkLength();
	CurrentCableLength = FMath::Clamp(
		FMath::Max(MinimumLength, TetherPath->GetRouteLength() + FMath::Max(0.0f, Chain->ChainSlack)),
		MinimumLength,
		Chain->MaxChainLength);
	TargetCableLength = CurrentCableLength;
	CableReelVelocity = 0.0f;
	DeployedChainLinkCount = FMath::CeilToInt(CurrentCableLength / GetFixedLinkLength());
	TargetChainLinkCount = DeployedChainLinkCount;
	LastValidPlayerLocation = PlayerActor->GetActorLocation();
	bHasLastValidPlayerLocation = true;
	RopeSimulation->InitializeRope(MachineAnchor, PlayerAnchor, CurrentCableLength, PlayerActor);
	RopeSimulation->SetRoutePath(TetherPath->GetRoutePoints(), TetherPath->GetRoutePointIds(), CurrentCableLength);
	UpdateChainVisuals();
	UE_LOG(LogChopIt, Display, TEXT("Hybrid tether created for %s with %.0f cm deployed."),
		*PlayerActor->GetName(), CurrentCableLength);
}

void AChopItQuotaMachine::UpdateRetractableChain(const float DeltaSeconds)
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain)
	{
		return;
	}
	if (!IsValid(ChainedPlayer))
	{
		TryCreatePlayerChain();
		return;
	}

	const FVector MachineAnchor = GetActorTransform().TransformPosition(Chain->MachineChainAnchor);
	FVector PlayerAnchor = ChainedPlayer->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);
	TetherPath->UpdatePath(MachineAnchor, PlayerAnchor);

	if (TetherPath->IsAtAnchorCapacity() && bHasLastValidPlayerLocation)
	{
		FVector Restore = LastValidPlayerLocation;
		Restore.Z = ChainedPlayer->GetActorLocation().Z;
		FHitResult Hit;
		ChainedPlayer->SetActorLocation(Restore, true, &Hit, ETeleportType::None);
		PlayerAnchor = ChainedPlayer->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);
		TetherPath->UpdatePath(MachineAnchor, PlayerAnchor);
	}

	float RouteLength = TetherPath->GetRouteLength();
	if (RouteLength >= Chain->MaxChainLength - FMath::Max(0.5f, Chain->ChainStretchTolerance))
	{
		CorrectHardLimit(RouteLength);
		PlayerAnchor = ChainedPlayer->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);
		TetherPath->UpdatePath(MachineAnchor, PlayerAnchor);
		RouteLength = TetherPath->GetRouteLength();
	}
	else if (!TetherPath->IsAtAnchorCapacity())
	{
		LastValidPlayerLocation = ChainedPlayer->GetActorLocation();
		bHasLastValidPlayerLocation = true;
	}

	UpdateReel(RouteLength, DeltaSeconds);
	UpdatePlayerTension(RouteLength);
	const float VisualRopeLength = FMath::Clamp(FMath::Max(CurrentCableLength, RouteLength), 1.0f, Chain->MaxChainLength);
	RopeSimulation->SetRoutePath(TetherPath->GetRoutePoints(), TetherPath->GetRoutePointIds(), VisualRopeLength);
	RopeSimulation->Simulate(DeltaSeconds);
	UpdateChainVisuals();
}

void AChopItQuotaMachine::UpdateReel(const float RouteLength, const float DeltaSeconds)
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain)
	{
		return;
	}
	const float MinimumLength = FMath::Clamp(Chain->MinimumDeployedLinks, 3, Chain->ChainLinkCount) * GetFixedLinkLength();
	float DesiredLength = FMath::Clamp(RouteLength + FMath::Max(0.0f, Chain->ChainSlack), MinimumLength, Chain->MaxChainLength);
	if (DesiredLength < TargetCableLength
		&& TargetCableLength - DesiredLength < FMath::Max(0.0f, Chain->ChainReelHysteresis))
	{
		DesiredLength = TargetCableLength;
	}
	TargetCableLength = DesiredLength;
	TargetChainLinkCount = FMath::Clamp(
		FMath::CeilToInt(TargetCableLength / GetFixedLinkLength()),
		FMath::Clamp(Chain->MinimumDeployedLinks, 3, Chain->ChainLinkCount),
		FMath::Clamp(Chain->ChainLinkCount, 3, 64));

	// The gameplay route may grow abruptly when it wraps. Feeding to the exact
	// route is immediate, so reel acceleration can never make the player heavy.
	CurrentCableLength = FMath::Max(CurrentCableLength, FMath::Min(RouteLength, Chain->MaxChainLength));
	const float Remaining = TargetCableLength - CurrentCableLength;
	const float Acceleration = FMath::Max(20.0f, Chain->ChainFeedAcceleration);
	float MaximumSpeed = FMath::Max(20.0f, Chain->ChainFeedSpeed);
	if (const ACharacter* Character = Cast<ACharacter>(ChainedPlayer))
	{
		if (const UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			MaximumSpeed = FMath::Max(MaximumSpeed, Movement->MaxWalkSpeed * 1.25f);
		}
	}
	if (FMath::Abs(Remaining) <= 0.25f)
	{
		CurrentCableLength = TargetCableLength;
		CableReelVelocity = 0.0f;
	}
	else
	{
		const float BrakingSpeed = FMath::Sqrt(2.0f * Acceleration * FMath::Abs(Remaining));
		const float DesiredVelocity = FMath::Sign(Remaining) * FMath::Min(MaximumSpeed, BrakingSpeed);
		CableReelVelocity = FMath::FInterpConstantTo(CableReelVelocity, DesiredVelocity, DeltaSeconds, Acceleration);
		const float Previous = CurrentCableLength;
		CurrentCableLength = FMath::Clamp(CurrentCableLength + CableReelVelocity * DeltaSeconds, MinimumLength, Chain->MaxChainLength);
		if ((TargetCableLength - Previous) * (TargetCableLength - CurrentCableLength) <= 0.0f)
		{
			CurrentCableLength = TargetCableLength;
			CableReelVelocity = 0.0f;
		}
	}
	DeployedChainLinkCount = FMath::Clamp(
		FMath::CeilToInt(CurrentCableLength / GetFixedLinkLength()),
		FMath::Clamp(Chain->MinimumDeployedLinks, 3, Chain->ChainLinkCount),
		FMath::Clamp(Chain->ChainLinkCount, 3, 64));
}

void AChopItQuotaMachine::UpdatePlayerTension(const float RouteLength)
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain)
	{
		return;
	}
	const float Band = FMath::Max(1.0f, Chain->TensionSoftBand);
	const float TensionAlpha = FMath::Clamp((RouteLength - (Chain->MaxChainLength - Band)) / Band, 0.0f, 1.0f);
	bHardLimited = TetherPath->IsAtAnchorCapacity()
		|| RouteLength >= Chain->MaxChainLength - FMath::Max(0.5f, Chain->ChainStretchTolerance);
	if (TetherReceiver)
	{
		TetherReceiver->SetTetherState(
			TetherPath->GetFinalGuidePoint(),
			TensionAlpha,
			bHardLimited,
			Chain->PlayerPullAcceleration,
			Chain->PlayerPullDamping);
	}
	TetherPath->ApplyTensionToPhysicsProps(
		TensionAlpha,
		Chain->MaximumPropTensionForce,
		Chain->PhysicsPropForceScale);
}

void AChopItQuotaMachine::CorrectHardLimit(const float RouteLength)
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain || !IsValid(ChainedPlayer) || RouteLength <= Chain->MaxChainLength)
	{
		return;
	}
	const FVector Guide = TetherPath->GetFinalGuidePoint();
	const float AvailableFinalSpan = FMath::Max(0.0f,
		Chain->MaxChainLength - TetherPath->GetPrefixLengthBeforeFinalSpan());
	const FVector CurrentActorLocation = ChainedPlayer->GetActorLocation();
	const FVector CurrentAnchor = ChainedPlayer->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);
	const FVector AnchorOffset = CurrentAnchor - CurrentActorLocation;
	const float VerticalDelta = CurrentAnchor.Z - Guide.Z;
	const float MaximumHorizontal = FMath::Sqrt(FMath::Max(0.0f,
		FMath::Square(AvailableFinalSpan) - FMath::Square(VerticalDelta)));
	FVector Horizontal = CurrentAnchor - Guide;
	Horizontal.Z = 0.0f;
	const float HorizontalDistance = Horizontal.Size();
	if (HorizontalDistance <= MaximumHorizontal || HorizontalDistance <= UE_SMALL_NUMBER)
	{
		return;
	}
	const FVector Outward = Horizontal / HorizontalDistance;
	FVector CorrectedAnchor = Guide + Outward * MaximumHorizontal;
	CorrectedAnchor.Z = CurrentAnchor.Z;
	FVector CorrectedActor = CorrectedAnchor - AnchorOffset;
	CorrectedActor.Z = CurrentActorLocation.Z;
	FHitResult Hit;
	ChainedPlayer->SetActorLocation(CorrectedActor, true, &Hit, ETeleportType::None);
	if (ACharacter* Character = Cast<ACharacter>(ChainedPlayer))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			const float OutwardSpeed = FVector::DotProduct(Movement->Velocity, Outward);
			if (OutwardSpeed > 0.0f)
			{
				Movement->Velocity -= Outward * OutwardSpeed;
			}
		}
	}
}

void AChopItQuotaMachine::UpdateChainVisuals()
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain || !RopeSimulation || !RopeSimulation->IsInitialized()
		|| !ChainLinkVisuals || !ChainLinkVisuals->GetStaticMesh() || CurrentCableLength <= UE_SMALL_NUMBER)
	{
		return;
	}
	const TArray<FVector>& Points = RopeSimulation->GetParticleLocations();
	if (Points.Num() < 2)
	{
		return;
	}
	const float PathLength = RopeSimulation->GetSimulatedPathLength();
	if (PathLength <= UE_SMALL_NUMBER)
	{
		return;
	}
	const float LinkLength = GetFixedLinkLength();
	int32 CompleteLinks = FMath::Clamp(FMath::FloorToInt(CurrentCableLength / LinkLength), 0, Chain->ChainLinkCount);
	float PartialLength = CurrentCableLength - CompleteLinks * LinkLength;
	if (PartialLength < 0.5f || CompleteLinks >= Chain->ChainLinkCount)
	{
		PartialLength = 0.0f;
	}
	TArray<float> VisualLengths;
	if (PartialLength > 0.0f)
	{
		VisualLengths.Add(PartialLength);
	}
	for (int32 Index = 0; Index < CompleteLinks; ++Index)
	{
		VisualLengths.Add(LinkLength);
	}

	ChainLinkVisuals->ClearInstances();
	float RestCursor = 0.0f;
	for (int32 VisualIndex = 0; VisualIndex < VisualLengths.Num(); ++VisualIndex)
	{
		const float VisualLength = VisualLengths[VisualIndex];
		const float SampleDistance = ((RestCursor + VisualLength * 0.5f) / CurrentCableLength) * PathLength;
		FVector Location;
		FVector Direction;
		if (SampleCableAtDistance(Points, SampleDistance, Location, Direction))
		{
			const FQuat Alignment = FRotationMatrix::MakeFromZ(Direction).ToQuat();
			const FQuat AlternatingTwist(FVector::UpVector, (VisualIndex % 2) * UE_HALF_PI);
			ChainLinkVisuals->AddInstance(FTransform(
				Alignment * AlternatingTwist,
				Location,
				FVector(
					Chain->ChainLinkThickness / 100.0f,
					Chain->ChainLinkThickness / 100.0f,
					(VisualLength + Chain->ChainLinkVisualOverlap) / 100.0f)), true);
		}
		RestCursor += VisualLength;
	}
}

bool AChopItQuotaMachine::SampleCableAtDistance(
	const TArray<FVector>& Points,
	const float Distance,
	FVector& OutLocation,
	FVector& OutDirection) const
{
	float Traversed = 0.0f;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		const FVector Segment = Points[Index] - Points[Index - 1];
		const float SegmentLength = Segment.Size();
		if (SegmentLength <= UE_SMALL_NUMBER)
		{
			continue;
		}
		if (Traversed + SegmentLength >= Distance)
		{
			OutLocation = FMath::Lerp(Points[Index - 1], Points[Index],
				FMath::Clamp((Distance - Traversed) / SegmentLength, 0.0f, 1.0f));
			OutDirection = Segment / SegmentLength;
			return true;
		}
		Traversed += SegmentLength;
	}
	OutLocation = Points.Last();
	OutDirection = (Points.Last() - Points[Points.Num() - 2]).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	return true;
}

float AChopItQuotaMachine::GetFixedLinkLength() const
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain)
	{
		return 1.0f;
	}
	const int32 MaximumLinks = FMath::Clamp(Chain->ChainLinkCount, 3, 64);
	return FMath::Max(Chain->ChainLinkLength, Chain->MaxChainLength / static_cast<float>(MaximumLinks));
}

const UChopItChainDefinition* AChopItQuotaMachine::GetChainDefinition() const
{
	if (ChainDefinition)
	{
		return ChainDefinition.Get();
	}
	return LoadObject<UChopItChainDefinition>(nullptr,
		TEXT("/Game/ChopIt/World/ChainLab/DA_Chain_Default.DA_Chain_Default"));
}

void AChopItQuotaMachine::DestroyPlayerChain()
{
	if (TetherReceiver)
	{
		TetherReceiver->ClearTetherState();
	}
	if (TetherPath)
	{
		TetherPath->ResetPath();
	}
	if (RopeSimulation)
	{
		RopeSimulation->ResetRope();
	}
	if (ChainLinkVisuals)
	{
		ChainLinkVisuals->ClearInstances();
	}
	ChainedPlayer = nullptr;
	TetherReceiver = nullptr;
	DeployedChainLinkCount = 0;
	TargetChainLinkCount = 0;
	CurrentCableLength = 0.0f;
	TargetCableLength = 0.0f;
	CableReelVelocity = 0.0f;
	bHardLimited = false;
	bHasLastValidPlayerLocation = false;
}
