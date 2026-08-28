#include "Economy/ChopItQuotaMachine.h"

#include "CableComponent.h"
#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Economy/ChopItChainDefinition.h"
#include "Economy/ChopItQuotaComponent.h"
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

	ChainCable = CreateDefaultSubobject<UCableComponent>(TEXT("PlayerChainCable"));
	ChainCable->SetupAttachment(SceneRoot);
	ChainCable->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	ChainCable->bAttachStart = true;
	ChainCable->bAttachEnd = true;
	ChainCable->CableLength = 400.0f;
	ChainCable->NumSegments = 192;
	ChainCable->SolverIterations = 16;
	ChainCable->SubstepTime = 0.008333f;
	ChainCable->bUseSubstepping = true;
	ChainCable->bEnableCollision = true;
	ChainCable->CollisionFriction = 0.0f;
	ChainCable->CableGravityScale = 1.0f;
	ChainCable->CableWidth = 12.5f;
	ChainCable->SetCollisionProfileName(ChopItCollisionProfiles::Chain);
	ChainCable->SetVisibility(false);
	ChainCable->SetHiddenInGame(true);
	ChainCable->PrimaryComponentTick.TickGroup = TG_PostPhysics;

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
	AddTickPrerequisiteComponent(ChainCable);
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
	const int32 SimulationSegments = FMath::Clamp(Chain->ChainLinkCount, 3, 64)
		* FMath::Clamp(Chain->CableSegmentsPerLink, 2, 6);
	if (ChainCable->NumSegments != SimulationSegments)
	{
		ChainCable->NumSegments = SimulationSegments;
		ChainCable->ReregisterComponent();
	}
	ChainCable->SolverIterations = FMath::Clamp(Chain->CableSolverIterations, 1, 16);
	ChainCable->SubstepTime = FMath::Clamp(Chain->CableSubstepTime, 0.005f, 0.033f);
	// Collision is sampled at particles rather than along the rendered mesh.
	// This separate setting lets art change the link thickness without opening
	// collision gaps between cable particles.
	ChainCable->CableWidth = FMath::Max(2.0f, Chain->CableParticleDiameter);
	ChainCable->bEnableCollision = Chain->bCableWorldCollision;
	ChainCable->CollisionFriction = FMath::Clamp(Chain->CableCollisionFriction, 0.0f, 1.0f);
	ChainCable->CableGravityScale = FMath::Max(0.0f, Chain->CableGravityScale)
		* (FMath::Max(0.01f, Chain->ChainLinkWeight) / 1.25f);
	ChainCable->SetRelativeLocation(Chain->MachineChainAnchor);
	if (Chain->ChainLinkMesh)
	{
		ChainLinkVisuals->SetStaticMesh(Chain->ChainLinkMesh);
	}
	ChainCable->SetComponentTickEnabled(false);

	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	UChopItQuotaComponent* Quota = GameState ? GameState->FindComponentByClass<UChopItQuotaComponent>() : nullptr;
	if (Quota)
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
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!IsValid(PlayerPawn) || !PlayerPawn->GetRootComponent())
	{
		if (World)
		{
			World->GetTimerManager().SetTimer(ChainCreationTimer, this, &AChopItQuotaMachine::TryCreatePlayerChain, 0.2f, false);
		}
		return;
	}
	const FVector MachineAnchorWorld = GetActorTransform().TransformPosition(Chain->MachineChainAnchor);
	const FVector PlayerAnchorWorld = PlayerPawn->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);
	CreatePlayerChain(PlayerPawn, CalculateRequiredLinkCount(FVector::Dist2D(MachineAnchorWorld, PlayerAnchorWorld)));
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
	const FVector MachineAnchorWorld = GetActorTransform().TransformPosition(Chain->MachineChainAnchor);
	EnforceCableTension(ChainedPlayer);
	EnforceMaximumCableLength(ChainedPlayer);
	const FVector AcceptedPlayerAnchor = ChainedPlayer->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);
	UpdateReel(FVector::Dist2D(MachineAnchorWorld, AcceptedPlayerAnchor), DeltaSeconds);
	UpdateChainVisuals();
}

void AChopItQuotaMachine::UpdateReel(const float HorizontalDistance, const float DeltaSeconds)
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain || !ChainCable)
	{
		return;
	}

	int32 RequiredLinks = CalculateRequiredLinkCount(HorizontalDistance);
	if (TensionMinimumLinkCount > 0
		&& HorizontalDistance < TensionReleaseReferenceDistance - FMath::Max(20.0f, Chain->ChainReelHysteresis))
	{
		TensionMinimumLinkCount = 0;
	}
	RequiredLinks = FMath::Max(RequiredLinks, TensionMinimumLinkCount);
	if (RequiredLinks < TargetChainLinkCount)
	{
		const float RetractionThreshold = (TargetChainLinkCount - 1) * GetFixedLinkLength()
			- FMath::Max(0.0f, Chain->ChainReelHysteresis);
		if (HorizontalDistance + FMath::Max(0.0f, Chain->ChainSlack) > RetractionThreshold)
		{
			RequiredLinks = TargetChainLinkCount;
		}
	}
	if (RequiredLinks != TargetChainLinkCount)
	{
		TargetChainLinkCount = RequiredLinks;
		TargetCableLength = TargetChainLinkCount * GetFixedLinkLength();
		UE_LOG(LogChopIt, Verbose, TEXT("Cable reel target changed to %d links."), TargetChainLinkCount);
	}

	const float RemainingLength = TargetCableLength - CurrentCableLength;
	const float Acceleration = FMath::Max(20.0f, Chain->ChainFeedAcceleration);
	if (FMath::Abs(RemainingLength) <= 0.25f && FMath::Abs(CableReelVelocity) <= 1.0f)
	{
		CurrentCableLength = TargetCableLength;
		CableReelVelocity = 0.0f;
		ChainCable->CableLength = CurrentCableLength;
		DeployedChainLinkCount = TargetChainLinkCount;
		return;
	}

	float MaximumFeedSpeed = FMath::Max(20.0f, Chain->ChainFeedSpeed);
	if (const ACharacter* Character = Cast<ACharacter>(ChainedPlayer))
	{
		if (const UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			MaximumFeedSpeed = FMath::Max(MaximumFeedSpeed, Movement->MaxWalkSpeed * 1.1f);
		}
	}
	const float BrakingSpeed = FMath::Sqrt(2.0f * Acceleration * FMath::Abs(RemainingLength));
	const float DesiredVelocity = FMath::Sign(RemainingLength)
		* FMath::Min(MaximumFeedSpeed, BrakingSpeed);
	CableReelVelocity = FMath::FInterpConstantTo(
		CableReelVelocity,
		DesiredVelocity,
		DeltaSeconds,
		Acceleration);
	const float PreviousLength = CurrentCableLength;
	CurrentCableLength += CableReelVelocity * DeltaSeconds;
	if ((TargetCableLength - PreviousLength) * (TargetCableLength - CurrentCableLength) <= 0.0f)
	{
		CurrentCableLength = TargetCableLength;
		CableReelVelocity = 0.0f;
	}
	ChainCable->CableLength = CurrentCableLength;
	DeployedChainLinkCount = FMath::Clamp(
		FMath::CeilToInt((CurrentCableLength - 0.5f) / GetFixedLinkLength()),
		FMath::Clamp(Chain->MinimumDeployedLinks, 3, Chain->ChainLinkCount),
		FMath::Clamp(Chain->ChainLinkCount, 3, 64));
}

void AChopItQuotaMachine::EnforceCableTension(AActor* PlayerActor)
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain || !IsValid(PlayerActor) || !ChainCable || !ChainCable->IsComponentTickEnabled())
	{
		return;
	}
	const FVector CurrentLocation = PlayerActor->GetActorLocation();
	if (!bHasLastAcceptedPlayerLocation)
	{
		LastAcceptedPlayerLocation = CurrentLocation;
		bHasLastAcceptedPlayerLocation = true;
		return;
	}

	TArray<FVector> Points;
	ChainCable->GetCableParticleLocations(Points);
	if (Points.Num() < 2)
	{
		LastAcceptedPlayerLocation = CurrentLocation;
		return;
	}
	FVector OutwardDirection = Points.Last() - Points[Points.Num() - 2];
	const float EndSegmentLength = OutwardDirection.Size();
	const float DesiredSegmentLength = CurrentCableLength / FMath::Max(1, ChainCable->NumSegments);
	const float EndSegmentStretch = EndSegmentLength - DesiredSegmentLength;
	float SimulatedPathLength = 0.0f;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		SimulatedPathLength += FVector::Distance(Points[Index - 1], Points[Index]);
	}
	const float TotalStretch = SimulatedPathLength - CurrentCableLength;
	const float LocalStretchTolerance = FMath::Max(1.0f, Chain->ChainStretchTolerance);
	const float TotalStretchTolerance = FMath::Max(25.0f, LocalStretchTolerance * 2.5f);
	OutwardDirection.Z = 0.0f;
	OutwardDirection = OutwardDirection.GetSafeNormal();
	const float OutwardMovement = FVector::DotProduct(CurrentLocation - LastAcceptedPlayerLocation, OutwardDirection);
	const bool bCableTensioned = EndSegmentStretch > LocalStretchTolerance
		|| TotalStretch > TotalStretchTolerance;
	if (bCableTensioned
		&& OutwardMovement > UE_SMALL_NUMBER
		&& !OutwardDirection.IsNearlyZero())
	{
		const int32 MaximumLinks = FMath::Clamp(Chain->ChainLinkCount, 3, 64);
		const bool bMaximumLengthIsOut = TargetChainLinkCount >= MaximumLinks
			&& CurrentCableLength >= Chain->MaxChainLength - 1.0f;
		if (!bMaximumLengthIsOut)
		{
			const float LinkLength = GetFixedLinkLength();
			if (TargetChainLinkCount < MaximumLinks
				&& CurrentCableLength >= TargetCableLength - LinkLength * 0.15f)
			{
				TensionMinimumLinkCount = FMath::Max(TensionMinimumLinkCount, TargetChainLinkCount + 1);
			}
			else
			{
				TensionMinimumLinkCount = FMath::Max(TensionMinimumLinkCount, TargetChainLinkCount);
			}
			const FVector MachineAnchor = GetActorTransform().TransformPosition(Chain->MachineChainAnchor);
			const FVector PlayerAnchor = PlayerActor->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);
			TensionReleaseReferenceDistance = FVector::Dist2D(MachineAnchor, PlayerAnchor);
			LastAcceptedPlayerLocation = CurrentLocation;
			UE_LOG(
				LogChopIt,
				VeryVerbose,
				TEXT("Cable tension requested another link; minimum=%d."),
				TensionMinimumLinkCount);
			return;
		}

		FVector CorrectedLocation = CurrentLocation - OutwardDirection * OutwardMovement;
		CorrectedLocation.Z = CurrentLocation.Z;
		PlayerActor->SetActorLocation(CorrectedLocation, true, nullptr, ETeleportType::None);
		if (ACharacter* Character = Cast<ACharacter>(PlayerActor))
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				const float OutwardSpeed = FVector::DotProduct(Movement->Velocity, OutwardDirection);
				if (OutwardSpeed > 0.0f)
				{
					Movement->Velocity -= OutwardDirection * OutwardSpeed;
				}
			}
		}
		UE_LOG(
			LogChopIt,
			VeryVerbose,
			TEXT("Cable blocked outward movement: endpointStretch=%.1f totalStretch=%.1f."),
			EndSegmentStretch,
			TotalStretch);
	}
	LastAcceptedPlayerLocation = PlayerActor->GetActorLocation();
}

void AChopItQuotaMachine::EnforceMaximumCableLength(AActor* PlayerActor)
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain || !IsValid(PlayerActor))
	{
		return;
	}
	const float MovementLimit = FMath::Max(100.0f, Chain->MaxChainLength - FMath::Max(1.0f, Chain->ChainLinkThickness));
	const FVector MachineAnchorWorld = GetActorTransform().TransformPosition(Chain->MachineChainAnchor);
	const FVector PlayerAnchorWorld = PlayerActor->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);
	FVector HorizontalOffset = PlayerAnchorWorld - MachineAnchorWorld;
	HorizontalOffset.Z = 0.0f;
	const float Distance = HorizontalOffset.Size();
	if (Distance <= MovementLimit || Distance <= UE_SMALL_NUMBER)
	{
		if (bWasChainAtLimit && Distance < MovementLimit - 5.0f)
		{
			bWasChainAtLimit = false;
		}
		return;
	}
	bWasChainAtLimit = true;
	const FVector OutwardDirection = HorizontalOffset / Distance;
	const FVector PlayerAnchorOffset = PlayerAnchorWorld - PlayerActor->GetActorLocation();
	FVector ClampedLocation = MachineAnchorWorld + OutwardDirection * MovementLimit - PlayerAnchorOffset;
	ClampedLocation.Z = PlayerActor->GetActorLocation().Z;
	PlayerActor->SetActorLocation(ClampedLocation, true, nullptr, ETeleportType::None);
	if (ACharacter* Character = Cast<ACharacter>(PlayerActor))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			const float OutwardSpeed = FVector::DotProduct(Movement->Velocity, OutwardDirection);
			if (OutwardSpeed > 0.0f)
			{
				Movement->Velocity -= OutwardDirection * OutwardSpeed;
			}
		}
	}
}

int32 AChopItQuotaMachine::CalculateRequiredLinkCount(const float HorizontalDistance) const
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain)
	{
		return 0;
	}
	const int32 MaximumLinks = FMath::Clamp(Chain->ChainLinkCount, 3, 64);
	const int32 MinimumLinks = FMath::Clamp(Chain->MinimumDeployedLinks, 3, MaximumLinks);
	return FMath::Clamp(FMath::CeilToInt((HorizontalDistance + FMath::Max(0.0f, Chain->ChainSlack)) / GetFixedLinkLength()), MinimumLinks, MaximumLinks);
}

void AChopItQuotaMachine::CreatePlayerChain(AActor* PlayerActor, const int32 DesiredLinkCount)
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	USceneComponent* PlayerRoot = IsValid(PlayerActor) ? PlayerActor->GetRootComponent() : nullptr;
	if (!Chain || !PlayerRoot || !ChainCable || !ChainLinkVisuals || !ChainLinkVisuals->GetStaticMesh())
	{
		return;
	}
	DestroyPlayerChain();
	ChainedPlayer = PlayerActor;
	DeployedChainLinkCount = FMath::Clamp(DesiredLinkCount, 3, FMath::Clamp(Chain->ChainLinkCount, 3, 64));
	TargetChainLinkCount = DeployedChainLinkCount;
	CurrentCableLength = DeployedChainLinkCount * GetFixedLinkLength();
	TargetCableLength = CurrentCableLength;
	CableReelVelocity = 0.0f;
	TensionMinimumLinkCount = 0;
	TensionReleaseReferenceDistance = 0.0f;
	ChainCable->SetRelativeLocation(Chain->MachineChainAnchor);
	ChainCable->SetAttachEndToComponent(PlayerRoot);
	const FVector PlayerAnchorWorld = PlayerActor->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);
	ChainCable->EndLocation = PlayerRoot->GetComponentTransform().InverseTransformPosition(PlayerAnchorWorld);
	ChainCable->bAttachStart = true;
	ChainCable->bAttachEnd = true;
	ChainCable->CableLength = CurrentCableLength;
	ChainCable->CollisionFriction = FMath::Clamp(Chain->CableCollisionFriction, 0.0f, 1.0f);
	ChainCable->TeleportDistanceThreshold = 0.0f;
	ChainCable->TeleportRotationThreshold = 0.0f;
	ChainCable->SetComponentTickEnabled(true);
	ChainLinkVisuals->SetStaticMesh(Chain->ChainLinkMesh ? Chain->ChainLinkMesh : ChainLinkVisuals->GetStaticMesh());
	LastAcceptedPlayerLocation = PlayerActor->GetActorLocation();
	bHasLastAcceptedPlayerLocation = true;
	UpdateChainVisuals();
	UE_LOG(LogChopIt, Verbose, TEXT("Verlet player chain created: visualLinks=%d, particles=%d, machine=%s, player=%s."),
		DeployedChainLinkCount, ChainCable->NumSegments + 1, *GetName(), *PlayerActor->GetName());
}

void AChopItQuotaMachine::UpdateChainVisuals()
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain || !ChainCable || !ChainLinkVisuals || !ChainLinkVisuals->GetStaticMesh() || CurrentCableLength <= UE_SMALL_NUMBER)
	{
		return;
	}
	TArray<FVector> Points;
	ChainCable->GetCableParticleLocations(Points);
	if (Points.Num() < 2)
	{
		return;
	}
	float PathLength = 0.0f;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		PathLength += FVector::Distance(Points[Index - 1], Points[Index]);
	}
	if (PathLength <= UE_SMALL_NUMBER)
	{
		return;
	}

	const int32 MaximumLinks = FMath::Clamp(Chain->ChainLinkCount, 3, 64);
	const float LinkLength = GetFixedLinkLength();
	int32 CompleteLinks = FMath::FloorToInt(CurrentCableLength / LinkLength);
	float PartialLength = CurrentCableLength - CompleteLinks * LinkLength;
	if (PartialLength < 0.5f)
	{
		PartialLength = 0.0f;
	}
	CompleteLinks = FMath::Clamp(CompleteLinks, 0, MaximumLinks);
	TArray<float> VisualLengths;
	VisualLengths.Reserve(CompleteLinks + 1);
	if (PartialLength > 0.0f && CompleteLinks < MaximumLinks)
	{
		VisualLengths.Add(PartialLength);
	}
	for (int32 Index = 0; Index < CompleteLinks; ++Index)
	{
		VisualLengths.Add(LinkLength);
	}

	ChainLinkVisuals->ClearInstances();
	float RestCursor = 0.0f;
	for (const float VisualLength : VisualLengths)
	{
		const float SampleDistance = ((RestCursor + VisualLength * 0.5f) / CurrentCableLength) * PathLength;
		FVector Location;
		FVector Direction;
		if (SampleCableAtDistance(Points, SampleDistance, Location, Direction))
		{
			const FTransform InstanceTransform(FRotationMatrix::MakeFromZ(Direction).ToQuat(), Location,
				FVector(
					Chain->ChainLinkThickness / 100.0f,
					Chain->ChainLinkThickness / 100.0f,
					(VisualLength + Chain->ChainLinkVisualOverlap) / 100.0f));
			ChainLinkVisuals->AddInstance(InstanceTransform, true);
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
			const float Alpha = FMath::Clamp((Distance - Traversed) / SegmentLength, 0.0f, 1.0f);
			OutLocation = FMath::Lerp(Points[Index - 1], Points[Index], Alpha);
			OutDirection = Segment / SegmentLength;
			return true;
		}
		Traversed += SegmentLength;
	}
	if (Points.Num() >= 2)
	{
		OutLocation = Points.Last();
		OutDirection = (Points.Last() - Points[Points.Num() - 2]).GetSafeNormal(
			UE_SMALL_NUMBER,
			FVector::ForwardVector);
		return true;
	}
	return false;
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
		return ChainDefinition;
	}
	return LoadObject<UChopItChainDefinition>(nullptr, TEXT("/Game/ChopIt/World/ChainLab/DA_Chain_Default.DA_Chain_Default"));
}

void AChopItQuotaMachine::DestroyPlayerChain()
{
	if (ChainCable)
	{
		ChainCable->bAttachEnd = false;
		ChainCable->SetAttachEndToComponent(nullptr);
		ChainCable->SetComponentTickEnabled(false);
	}
	if (ChainLinkVisuals)
	{
		ChainLinkVisuals->ClearInstances();
	}
	ChainedPlayer = nullptr;
	DeployedChainLinkCount = 0;
	TargetChainLinkCount = 0;
	CurrentCableLength = 0.0f;
	TargetCableLength = 0.0f;
	CableReelVelocity = 0.0f;
	TensionMinimumLinkCount = 0;
	TensionReleaseReferenceDistance = 0.0f;
	bWasChainAtLimit = false;
	bHasLastAcceptedPlayerLocation = false;
}
