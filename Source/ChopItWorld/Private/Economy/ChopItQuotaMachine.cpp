#include "Economy/ChopItQuotaMachine.h"

#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/ChopItCameraFacingTextComponent.h"
#include "Camera/ChopItCameraAnchor.h"
#include "Camera/ChopItCameraCue.h"
#include "Camera/ChopItCameraDirectorSubsystem.h"
#include "Core/CameraShakeAsset.h"
#include "Curves/CurveFloat.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Economy/ChopItChainDefinition.h"
#include "Economy/ChopItDeathRemains.h"
#include "Economy/ChopItQuotaComponent.h"
#include "Economy/ChopItRopeComponent.h"
#include "Economy/ChopItTetherPathComponent.h"
#include "Economy/ChopItTetherReceiverComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	thread_local int32 GDeathCollisionQueries = 0;
}

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
	MachineVisual->SetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid, ECR_Ignore);

	QuotaLabel = CreateDefaultSubobject<UChopItCameraFacingTextComponent>(TEXT("QuotaLabel"));
	QuotaLabel->SetupAttachment(SceneRoot);
	QuotaLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 245.0f));
	QuotaLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	QuotaLabel->SetHorizontalAlignment(EHTA_Center);
	QuotaLabel->SetWorldSize(42.0f);
	QuotaLabel->SetTextRenderColor(FColor::Red);

	LeverLabel = CreateDefaultSubobject<UChopItCameraFacingTextComponent>(TEXT("LeverLabel"));
	LeverLabel->SetupAttachment(SceneRoot);
	LeverLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 295.0f));
	LeverLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	LeverLabel->SetHorizontalAlignment(EHTA_Center);
	LeverLabel->SetWorldSize(30.0f);
	LeverLabel->SetTextRenderColor(FColor::Silver);

	ReleasedChainLabel = CreateDefaultSubobject<UChopItCameraFacingTextComponent>(TEXT("ReleasedChainLabel"));
	ReleasedChainLabel->SetupAttachment(SceneRoot);
	ReleasedChainLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 340.0f));
	ReleasedChainLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	ReleasedChainLabel->SetHorizontalAlignment(EHTA_Center);
	ReleasedChainLabel->SetWorldSize(28.0f);
	ReleasedChainLabel->SetTextRenderColor(FColor::Cyan);

	DeliveryGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("DeliveryGlow"));
	DeliveryGlow->SetupAttachment(SceneRoot);
	DeliveryGlow->SetRelativeLocation(FVector(0.0f, 0.0f, 270.0f));
	DeliveryGlow->SetLightColor(FLinearColor(1.0f, 0.18f, 0.015f));
	DeliveryGlow->SetAttenuationRadius(520.0f);
	DeliveryGlow->SetIntensity(0.0f);
	DeliveryGlow->SetCastShadows(false);

	WoodChipPool = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PooledWoodChips"));
	WoodChipPool->SetupAttachment(SceneRoot);
	WoodChipPool->SetMobility(EComponentMobility::Movable);
	WoodChipPool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WoodChipPool->SetGenerateOverlapEvents(false);
	WoodChipPool->SetCastShadow(false);

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
		WoodChipPool->SetStaticMesh(CubeMesh.Object);
	}
	if (CylinderMesh.Succeeded())
	{
		ChainLinkVisuals->SetStaticMesh(CylinderMesh.Object);
	}
}

void AChopItQuotaMachine::BeginPlay()
{
	Super::BeginPlay();
	MachineBaseLocation = MachineVisual->GetRelativeLocation();
	MachineBaseScale = MachineVisual->GetRelativeScale3D();
	MachineBaseRotation = MachineVisual->GetRelativeRotation();
	DeliveryVisualRandom.Initialize(GetUniqueID() * 104729 + 31);
	WoodChips.SetNum(FMath::Max(32, WoodChipPoolSize));
	for (int32 Index = 0; Index < WoodChips.Num(); ++Index)
	{
		WoodChipPool->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -100000.0f), FVector::ZeroVector));
	}
	if (UMaterialInterface* WoodMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/ChopIt/World/Blockout/Materials/MI_Wood.MI_Wood")))
	{
		WoodChipPool->SetMaterial(0, WoodMaterial);
	}
	// Existing Blueprint defaults may predate the opt-in camera channel.
	MachineVisual->SetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid, ECR_Ignore);
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
		if (bDeathSlowMotionApplied) UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	}
	DestroyPlayerChain();
	Super::EndPlay(EndPlayReason);
}

void AChopItQuotaMachine::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateDeliveryReaction(DeltaSeconds);
	UpdateDeathCrush(DeltaSeconds);
	UpdateWoodChips(DeltaSeconds);
	if (DeliveryGlowRemaining > 0.0f)
	{
		DeliveryGlowRemaining = FMath::Max(0.0f, DeliveryGlowRemaining - DeltaSeconds);
		const float Alpha = DeliveryGlowRemaining / 0.22f;
		DeliveryGlow->SetIntensity((2200.0f + FMath::Sin(DeliveryGlowRemaining * 95.0f) * 450.0f) * Alpha);
	}
	else
	{
		DeliveryGlow->SetIntensity(0.0f);
	}
	if (const UChopItChainDefinition* Chain = GetChainDefinition(); Chain && Chain->bChainPlayerToMachine)
	{
		if (!TetherReceiver && !bChainedPlayerConsumed) UpdateRetractableChain(DeltaSeconds);
		TetherPath->Synchronize();
		UpdateChainVisuals();
	}
	UpdateReleasedChainLabel();
}

FVector AChopItQuotaMachine::GetDeliveryIntakeWorldLocation() const
{
	return GetActorTransform().TransformPosition(FVector(0.0f, 0.0f, 285.0f));
}

void AChopItQuotaMachine::NotifyWoodConsumed(const int32 Units)
{
	if (Units <= 0) return;
	DeliveryGlowRemaining = 0.22f;
	DeliveryGlow->SetIntensity(2800.0f);
	DeliveryReactionStrength = FMath::Clamp(
		DeliveryReactionStrength + 0.34f * static_cast<float>(Units), 0.0f, 1.0f);
	SpawnWoodChips(FMath::Clamp(Units * WoodChipsPerItem, 1, 20));
}

void AChopItQuotaMachine::UpdateDeliveryReaction(const float DeltaSeconds)
{
	DeliveryAnimationTime += DeltaSeconds;
	DeliveryReactionStrength = FMath::FInterpTo(DeliveryReactionStrength, 0.0f, DeltaSeconds, 3.2f);
	if (DeliveryReactionStrength <= 0.001f)
	{
		DeliveryReactionStrength = 0.0f;
		MachineVisual->SetRelativeLocation(MachineBaseLocation);
		MachineVisual->SetRelativeRotation(MachineBaseRotation);
		MachineVisual->SetRelativeScale3D(MachineBaseScale);
		return;
	}

	const float Chew = FMath::Sin(DeliveryAnimationTime * 23.0f);
	const float Grind = FMath::Sin(DeliveryAnimationTime * 34.0f + 0.7f);
	const float Compression = 0.5f + 0.5f * FMath::Abs(Chew);
	MachineVisual->SetRelativeLocation(MachineBaseLocation + FVector(
		Grind * 4.5f,
		Chew * 3.0f,
		FMath::Abs(Chew) * 10.0f) * DeliveryReactionStrength);
	MachineVisual->SetRelativeRotation(MachineBaseRotation + FRotator(
		Chew * 2.5f,
		Grind * 2.0f,
		FMath::Sin(DeliveryAnimationTime * 18.0f) * 5.0f) * DeliveryReactionStrength);
	MachineVisual->SetRelativeScale3D(FVector(
		MachineBaseScale.X * (1.0f + 0.085f * Compression * DeliveryReactionStrength),
		MachineBaseScale.Y * (1.0f + 0.085f * Compression * DeliveryReactionStrength),
		MachineBaseScale.Z * (1.0f - 0.065f * Compression * DeliveryReactionStrength)));
}

void AChopItQuotaMachine::SpawnWoodChips(const int32 Count)
{
	const FVector Intake = GetDeliveryIntakeWorldLocation();
	for (int32 SpawnIndex = 0; SpawnIndex < Count; ++SpawnIndex)
	{
		int32 PoolIndex = WoodChips.IndexOfByPredicate([](const FWoodChipParticle& Chip) { return !Chip.bActive; });
		if (PoolIndex == INDEX_NONE)
		{
			float OldestRatio = -1.0f;
			for (int32 Index = 0; Index < WoodChips.Num(); ++Index)
			{
				const float Ratio = WoodChips[Index].Age / FMath::Max(0.01f, WoodChips[Index].Lifetime);
				if (Ratio > OldestRatio) { OldestRatio = Ratio; PoolIndex = Index; }
			}
		}
		if (!WoodChips.IsValidIndex(PoolIndex)) return;

		FWoodChipParticle& Chip = WoodChips[PoolIndex];
		Chip.bActive = true;
		Chip.Location = Intake + FVector(
			DeliveryVisualRandom.FRandRange(-30.0f, 30.0f),
			DeliveryVisualRandom.FRandRange(-30.0f, 30.0f),
			DeliveryVisualRandom.FRandRange(-8.0f, 18.0f));
		Chip.Velocity = FVector(
			DeliveryVisualRandom.FRandRange(-210.0f, 210.0f),
			DeliveryVisualRandom.FRandRange(-210.0f, 210.0f),
			DeliveryVisualRandom.FRandRange(360.0f, 650.0f));
		Chip.Rotation = FRotator(
			DeliveryVisualRandom.FRandRange(0.0f, 360.0f),
			DeliveryVisualRandom.FRandRange(0.0f, 360.0f),
			DeliveryVisualRandom.FRandRange(0.0f, 360.0f));
		Chip.AngularVelocity = FRotator(
			DeliveryVisualRandom.FRandRange(-620.0f, 620.0f),
			DeliveryVisualRandom.FRandRange(-620.0f, 620.0f),
			DeliveryVisualRandom.FRandRange(-620.0f, 620.0f));
		const float Size = DeliveryVisualRandom.FRandRange(0.018f, 0.038f);
		Chip.BaseScale = FVector(Size, Size * DeliveryVisualRandom.FRandRange(0.55f, 1.0f), Size * DeliveryVisualRandom.FRandRange(1.3f, 2.8f));
		Chip.Age = 0.0f;
		Chip.Lifetime = DeliveryVisualRandom.FRandRange(0.62f, 1.05f);
		WoodChipPool->UpdateInstanceTransform(PoolIndex,
			FTransform(Chip.Rotation, Chip.Location, Chip.BaseScale), true, false, true);
	}
	WoodChipPool->MarkRenderStateDirty();
}

void AChopItQuotaMachine::UpdateWoodChips(const float DeltaSeconds)
{
	bool bChanged = false;
	for (int32 Index = 0; Index < WoodChips.Num(); ++Index)
	{
		FWoodChipParticle& Chip = WoodChips[Index];
		if (!Chip.bActive) continue;
		Chip.Age += DeltaSeconds;
		if (Chip.Age >= Chip.Lifetime)
		{
			HideWoodChip(Index);
			bChanged = true;
			continue;
		}
		Chip.Velocity.Z -= 920.0f * DeltaSeconds;
		Chip.Velocity *= FMath::Pow(0.82f, DeltaSeconds);
		Chip.Location += Chip.Velocity * DeltaSeconds;
		Chip.Rotation += Chip.AngularVelocity * DeltaSeconds;
		const float LifeAlpha = Chip.Age / Chip.Lifetime;
		const float ScaleAlpha = FMath::Clamp(1.0f - FMath::Square(LifeAlpha), 0.0f, 1.0f);
		WoodChipPool->UpdateInstanceTransform(Index,
			FTransform(Chip.Rotation, Chip.Location, Chip.BaseScale * ScaleAlpha), true, false, true);
		bChanged = true;
	}
	if (bChanged) WoodChipPool->MarkRenderStateDirty();
}

void AChopItQuotaMachine::HideWoodChip(const int32 PoolIndex)
{
	if (!WoodChips.IsValidIndex(PoolIndex)) return;
	WoodChips[PoolIndex].bActive = false;
	WoodChipPool->UpdateInstanceTransform(PoolIndex,
		FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -100000.0f), FVector::ZeroVector), false, false, true);
}

int32 AChopItQuotaMachine::GetActiveWoodChipCount() const
{
	int32 Count = 0;
	for (const FWoodChipParticle& Chip : WoodChips) Count += Chip.bActive ? 1 : 0;
	return Count;
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
		? FString::Printf(TEXT("QUOTA PAID  %d / %d"), Progress, Target)
		: FString::Printf(TEXT("QUOTA  %d / %d"), Progress, Target)));
	QuotaLabel->SetTextRenderColor(bComplete ? FColor::Green : FColor::Red);
	RefreshLeverLabel();
}

void AChopItQuotaMachine::HandlePhaseChanged(const EChopItCyclePhase, const EChopItCyclePhase, const int32)
{
	if (GetWorld())
	{
		if (AGameStateBase* GameState = GetWorld()->GetGameState())
		{
			if (const UChopItCycleStateMachineComponent* Cycle = GameState->FindComponentByClass<UChopItCycleStateMachineComponent>())
			{
				if (Cycle->GetCurrentPhase() == EChopItCyclePhase::Death) BeginDeathSequence();
			}
		}
	}
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
		LeverLabel->SetText(FText::FromString(TEXT("E: PULL LEVER")));
		LeverLabel->SetTextRenderColor(FColor::Green);
	}
	else if (Cycle && Cycle->GetCurrentPhase() == EChopItCyclePhase::Night)
	{
		LeverLabel->SetText(FText::FromString(TEXT("PAY THE QUOTA TO UNLOCK THE LEVER")));
		LeverLabel->SetTextRenderColor(FColor::Yellow);
	}
	else
	{
		LeverLabel->SetText(FText::FromString(TEXT("LEVER LOCKED")));
		LeverLabel->SetTextRenderColor(FColor::Silver);
	}
}

void AChopItQuotaMachine::TryCreatePlayerChain()
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain || !Chain->bChainPlayerToMachine || bChainedPlayerConsumed || IsValid(ChainedPlayer))
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
	if (!Chain || !IsValid(PlayerActor) || !RopeSimulation || !TetherPath || !ChainLinkVisuals) return;
	DestroyPlayerChain();
	ChainedPlayer = PlayerActor;
	TetherReceiver = PlayerActor->FindComponentByClass<UChopItTetherReceiverComponent>();
	const FVector Start = GetActorTransform().TransformPosition(Chain->MachineChainAnchor);
	const FVector End = PlayerActor->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);
	CurrentCableLength = FMath::Clamp(static_cast<float>(FVector::Distance(Start, End)) + Chain->ChainSlack,
		Chain->MinimumDeployedLength, Chain->MaxChainLength);
	RopeSimulation->InitializeRope(Start, End, CurrentCableLength, PlayerActor);
	RopeSimulation->SetAutomaticReel(true);
	TetherPath->SetSource(RopeSimulation);
	if (TetherReceiver) TetherReceiver->BindMachine(this);
	if (!RopeSimulation->IsInitialized())
		UE_LOG(LogChopIt, Warning, TEXT("Chain V2 outlet-to-player initialization is obstructed; waiting for a clear deployment."));
	UpdateChainVisuals();
}

void AChopItQuotaMachine::AdvancePlayerChain(float DeltaSeconds)
{
	if (bDeathSequenceActive)
	{
		UpdateDeathSequence(DeltaSeconds);
		return;
	}
	UpdateRetractableChain(DeltaSeconds);
}

void AChopItQuotaMachine::BeginDeathSequence()
{
	if (bDeathSequenceActive || bChainedPlayerConsumed) return;
	if (!IsValid(ChainedPlayer) || !RopeSimulation || !RopeSimulation->IsInitialized())
	{
		UE_LOG(LogChopIt, Warning, TEXT("Death began without an existing player chain; skipping chain pull"));
		BeginDefeatPresentation();
		return;
	}
	bDeathSequenceActive = true;
	RopeSimulation->SetDeathRetracting(true);
	const UChopItChainDefinition* Chain = GetChainDefinition();
	DeathStartRopeLength = RopeSimulation->GetRopeLength();
	DeathDesiredRopeLength = DeathStartRopeLength;
	CurrentDeathRetractionTime = 0.0f;
	DeathCurrentRetractionSpeed = 0.0f;
	DeathRetractionDebt = 0.0f;
	DeathBestDistance = Chain ? static_cast<float>(FVector::Dist2D(
		ChainedPlayer->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor),
		GetActorTransform().TransformPosition(Chain->MachineChainAnchor))) : TNumericLimits<float>::Max();
	DeathNoProgressTime = 0.0f;
	if (ACharacter* Character = Cast<ACharacter>(ChainedPlayer))
	{
		Character->DisableInput(Cast<APlayerController>(Character->GetController()));
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Falling);
		}
	}
	if (TetherReceiver) TetherReceiver->ClearTetherState();
	DeathBypassedActors.Reset();
	if (ACharacter* Character = Cast<ACharacter>(ChainedPlayer))
		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			Capsule->IgnoreActorWhenMoving(this, true);
			DeathBypassedActors.Add(this);
		}
	DeathStallCount = 0;
	DeliveryReactionStrength = 1.0f;
	DeliveryAnimationTime = 0.0f;
	DeliveryGlowRemaining = 1.6f;
	PlayDeathShake(0.18f);
}

void AChopItQuotaMachine::UpdateDeathSequence(const float DeltaSeconds)
{
	const double UpdateStart = FPlatformTime::Seconds();
	GDeathCollisionQueries = 0;
	ON_SCOPE_EXIT
	{
		DeathUpdateMilliseconds = static_cast<float>((FPlatformTime::Seconds() - UpdateStart) * 1000.0);
		DeathCollisionQueries = GDeathCollisionQueries;
	};
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain || !IsValid(ChainedPlayer) || !RopeSimulation || !RopeSimulation->IsInitialized()) return;
	const float Dt = FMath::Max(0.0f, DeltaSeconds);
	const FVector Start = GetActorTransform().TransformPosition(Chain->MachineChainAnchor);
	const FVector End = ChainedPlayer->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);

	// A curve value is a speed blend, never a direct rope-length blend.
	CurrentDeathRetractionTime += Dt;
	const float CurveAlpha = Chain->DeathRetractionCurve
		? FMath::Clamp(Chain->DeathRetractionCurve->GetFloatValue(CurrentDeathRetractionTime), 0.0f, 1.0f)
		: FMath::SmoothStep(0.0f, FMath::Max(0.01f, Chain->DeathRetractionDuration), CurrentDeathRetractionTime);
	const float MinSpeed = FMath::Max(0.0f, Chain->DeathRetractionMinSpeed);
	const float MaxSpeed = FMath::Max(MinSpeed, Chain->DeathRetractionMaxSpeed);
	DeathCurrentRetractionSpeed = FMath::Lerp(MinSpeed, MaxSpeed, CurveAlpha);
	// The target keeps shrinking at the commanded speed even while the pawn is blocked.
	DeathDesiredRopeLength = FMath::Max(Chain->MinimumDeployedLength,
		DeathDesiredRopeLength - DeathCurrentRetractionSpeed * Dt);
	RopeSimulation->SetEndpoints(Start, End);
	RopeSimulation->SetDeathAllowedLength(DeathDesiredRopeLength);
	RopeSimulation->Simulate(Dt);
	ApplyTetherConstraint(End, RopeSimulation->GetDeathConstrainedEndpoint(), Dt);
	const FVector ConstrainedEnd = ChainedPlayer->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);
	RopeSimulation->AttachDeathEndpoint(ConstrainedEnd);
	const float ActualLength = RopeSimulation->GetRopeLength();
	DeathRetractionDebt = FMath::Max(0.0f, RopeSimulation->GetSimulatedPathLength() - ActualLength);

	CurrentCableLength = ActualLength;
	TetherPath->Synchronize();
	DeliveryReactionStrength = 1.0f;
	DeliveryGlowRemaining = FMath::Max(DeliveryGlowRemaining, 0.2f);

	const float Distance = static_cast<float>(FVector::Dist2D(ConstrainedEnd, Start));
	if (Distance <= Chain->DeathIntakeRadius)
	{
		ConsumeChainedPlayer();
		return;
	}
	if (Distance < DeathBestDistance - Chain->DeathPullMinProgress)
	{
		DeathBestDistance = Distance;
		DeathNoProgressTime = 0.0f;
	}
	else DeathNoProgressTime += Dt;

	if (Chain->bDebugDeathPull)
		UE_LOG(LogChopIt, Display, TEXT("Death tether distance %.1f target %.1f deployed %.1f excess %.1f speed %.1f endpoint %.1f"),
			Distance, DeathDesiredRopeLength, ActualLength, DeathRetractionDebt,
			DeathCurrentRetractionSpeed, FVector::Distance(ConstrainedEnd, RopeSimulation->GetAcceptedEndpoint()));

	// A sealed geometry pocket cannot be solved by the tether. End the cinematic
	// after a long stall without moving or teleporting the character.
	if (DeathNoProgressTime < FMath::Max(2.5f, Chain->DeathObstacleBypassDelay)) return;
	++DeathStallCount;
	bDeathSequenceActive = false;
	if (ACharacter* Character = Cast<ACharacter>(ChainedPlayer))
		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			for (const TWeakObjectPtr<AActor>& Actor : DeathBypassedActors)
				if (Actor.IsValid()) Capsule->IgnoreActorWhenMoving(Actor.Get(), false);
	DeathBypassedActors.Reset();
	if (TetherReceiver)
	{
		TetherReceiver->ClearTetherState();
		TetherReceiver->BindMachine(nullptr);
	}
	UE_LOG(LogChopIt, Warning, TEXT("Death tether remained blocked; ending the cinematic at the player's location"));
	BeginDefeatPresentation();
}

void AChopItQuotaMachine::ConsumeChainedPlayer()
{
	if (bChainedPlayerConsumed || !IsValid(ChainedPlayer)) return;
	bChainedPlayerConsumed = true;
	bDeathSequenceActive = false;
	const FVector Mouth = GetActorTransform().TransformPosition(FVector(0.0f, 0.0f, 190.0f));
	const FVector Ejection = (ChainedPlayer->GetActorLocation() - Mouth).GetSafeNormal(UE_SMALL_NUMBER, -GetActorForwardVector());
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AChopItDeathRemains* Remains = World->SpawnActor<AChopItDeathRemains>(Mouth, FRotator::ZeroRotator, Parameters))
		{
			Remains->InitializeRemains(Ejection);
		}
	}
	if (ACharacter* Character = Cast<ACharacter>(ChainedPlayer))
	{
		Character->GetMesh()->SetVisibility(false, true);
	}
	else
	{
		ChainedPlayer->SetActorHiddenInGame(true);
	}
	ChainedPlayer->SetActorEnableCollision(false);
	DeliveryReactionStrength = 1.0f;
	DeliveryGlowRemaining = 1.6f;
	PlayDeathShake(0.46f);
	bDeathCrushActive = true;
	DeathCrushElapsed = 0.0f;
	DestroyPlayerChain();
}

void AChopItQuotaMachine::UpdateDeathCrush(const float DeltaSeconds)
{
	if (!bDeathCrushActive || bDeathPresentationReady) return;
	DeathCrushElapsed += FMath::Max(0.0f, DeltaSeconds);
	DeliveryReactionStrength = 1.0f;
	DeliveryGlowRemaining = FMath::Max(DeliveryGlowRemaining, 0.2f);
	// The first half is the visible swallow. Freeze the moment the grinder has
	// properly bitten down, rather than covering the whole pull with a defeat UI.
	if (DeathCrushElapsed >= 0.60f) BeginDefeatPresentation();
}

void AChopItQuotaMachine::BeginDefeatPresentation()
{
	if (bDeathPresentationReady) return;
	bDeathPresentationReady = true;
	bDeathSlowMotionApplied = true;
	UGameplayStatics::SetGlobalTimeDilation(this, 0.20f);
	UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	if (!World || !LocalPlayer) return;
	const FVector Mouth = GetActorTransform().TransformPosition(FVector(0.0f, 0.0f, 190.0f));
	const FVector CurrentCameraLocation = Controller->PlayerCameraManager
		? Controller->PlayerCameraManager->GetCameraLocation()
		: Mouth + GetActorTransform().TransformVectorNoScale(FVector(520.0f, -520.0f, 340.0f));
	// Move the current camera ten percent closer to the machine; this avoids the
	// abrupt close-up caused by the former fixed death-camera offset and FOV.
	const FVector CameraLocation = Mouth + (CurrentCameraLocation - Mouth) * 0.90f;
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	DeathCameraAnchor = World->SpawnActor<AChopItCameraAnchor>(CameraLocation, (Mouth - CameraLocation).Rotation(), Parameters);
	if (!DeathCameraAnchor) return;
	DeathCameraAnchor->DefaultSubject = this;
	DeathCameraAnchor->bUseActorLocationForSubject = true;
	DeathCameraAnchor->SubjectFocusOffset = Mouth - GetActorLocation();
	DeathCameraCue = NewObject<UChopItCameraCue>(this);
	DeathCameraCue->Mode = EChopItCameraMode::Death;
	DeathCameraCue->FieldOfView = Controller->PlayerCameraManager
		? Controller->PlayerCameraManager->GetFOVAngle()
		: 85.0f;
	DeathCameraCue->Priority = 2000;
	DeathCameraCue->InputLocks = static_cast<int32>(EChopItCameraInputLock::Camera | EChopItCameraInputLock::Actions);
	if (UChopItCameraDirectorSubsystem* Camera = LocalPlayer->GetSubsystem<UChopItCameraDirectorSubsystem>())
	{
		Camera->PushCameraCueWithFieldOfView(DeathCameraCue, DeathCameraAnchor, this, DeathCameraCue->FieldOfView, 0.18f);
	}
}

void AChopItQuotaMachine::PlayDeathShake(const float Scale)
{
	UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	const UCameraShakeAsset* Shake = LoadObject<UCameraShakeAsset>(nullptr,
		TEXT("/Game/ChopIt/Presentation/Camera/Shakes/CS_Critical.CS_Critical"));
	if (LocalPlayer && Shake)
	{
		if (UChopItCameraDirectorSubsystem* Camera = LocalPlayer->GetSubsystem<UChopItCameraDirectorSubsystem>())
		{
			Camera->PlayCameraShake(Shake, Scale, GetActorLocation());
		}
	}
}

void AChopItQuotaMachine::ApplyTetherConstraint(
	const FVector& CurrentEnd, const FVector& AcceptedEnd, const float DeltaSeconds)
{
	const FVector Correction = AcceptedEnd - CurrentEnd;
	if (Correction.IsNearlyZero(0.01f) || !IsValid(ChainedPlayer)) return;
	FHitResult Hit;
	// This is the existing swept tether constraint correction, shared by play
	// and death. The death controller never supplies a player movement target.
	ChainedPlayer->SetActorLocation(ChainedPlayer->GetActorLocation() + Correction,
		true, &Hit, ETeleportType::None);
	if (ACharacter* Character = Cast<ACharacter>(ChainedPlayer))
	{
		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		if (Hit.IsValidBlockingHit() && Hit.Time < 1.0f)
			static_cast<UMovementComponent*>(Movement)->SlideAlongSurface(
				Correction, 1.0f - Hit.Time, Hit.Normal, Hit, true);
		const bool bAtMaterialLimit = RopeSimulation->GetStoredLength() < 0.1f;
		const FVector BlockedDirection = bAtMaterialLimit
			? RopeSimulation->GetOutwardDirection() : -Correction.GetSafeNormal();
		const float Speed = FVector::DotProduct(Movement->Velocity, BlockedDirection);
		if (Speed > 0) Movement->Velocity -= BlockedDirection
			* (bAtMaterialLimit ? Speed : FMath::Min(Speed,
				static_cast<float>(Correction.Size()) / FMath::Max(DeltaSeconds, UE_SMALL_NUMBER)));
	}
}

void AChopItQuotaMachine::UpdateRetractableChain(float DeltaSeconds)
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain || !Chain->bChainPlayerToMachine) { DestroyPlayerChain(); return; }
	if (!IsValid(ChainedPlayer)) { TryCreatePlayerChain(); return; }
	const FVector Start = GetActorTransform().TransformPosition(Chain->MachineChainAnchor);
	const FVector End = ChainedPlayer->GetActorTransform().TransformPosition(Chain->PlayerChainAnchor);
	if (!RopeSimulation->IsInitialized())
	{
		RopeSimulation->InitializeRope(Start, End,
			FMath::Clamp(static_cast<float>(FVector::Distance(Start, End)) + Chain->ChainSlack,
				Chain->MinimumDeployedLength, Chain->MaxChainLength), ChainedPlayer);
		if (!RopeSimulation->IsInitialized()) return;
	}
	RopeSimulation->SetEndpoints(Start, End);
	RopeSimulation->Simulate(DeltaSeconds);
	ApplyTetherConstraint(End, RopeSimulation->GetAcceptedEndpoint(), DeltaSeconds);
	CurrentCableLength = RopeSimulation->GetRopeLength();
	// A frame-time cutoff preserves the last valid rope shape; it is not a
	// physical obstruction and must never remove the player's outward input.
	bHardLimited = RopeSimulation->IsMovementBlocked()
		&& !RopeSimulation->IsFrameBudgetLimited()
		&& RopeSimulation->GetStoredLength() < 0.1f;
	if (TetherReceiver)
	{
		// Translate the physical anchor direction into actor-origin coordinates.
		const FVector Guide = ChainedPlayer->GetActorLocation() - RopeSimulation->GetOutwardDirection() * 100.0f;
		TetherReceiver->SetTetherState(Guide,
			FMath::Clamp(RopeSimulation->GetEndpointTension() / FMath::Max(1.0f, Chain->MaximumPropTensionForce), 0.0f, 1.0f),
			bHardLimited);
	}
	RopeSimulation->ApplyForcesToPhysicsProps(DeltaSeconds);
	TetherPath->Synchronize();
}

void AChopItQuotaMachine::UpdateChainVisuals()
{
	const UChopItChainDefinition* Chain = GetChainDefinition();
	if (!Chain || !RopeSimulation || !RopeSimulation->IsInitialized() || !ChainLinkVisuals->GetStaticMesh()) return;
	const TArray<FVector>& Points = RopeSimulation->GetParticleLocations();
	const int32 Count = Points.Num() - 1;
	while (ChainLinkVisuals->GetInstanceCount() < Count) ChainLinkVisuals->AddInstance(FTransform::Identity);
	while (ChainLinkVisuals->GetInstanceCount() > Count) ChainLinkVisuals->RemoveInstance(ChainLinkVisuals->GetInstanceCount() - 1);
	const FBox MeshBounds = ChainLinkVisuals->GetStaticMesh()->GetBoundingBox();
	const FVector MeshSize = MeshBounds.GetSize();
	// Every visual link is contained in the capsule actually checked by the
	// solver. No chord spanning several physical edges can cut a corner.
	for (int32 I = 0; I < Count; ++I)
	{
		const FVector Edge = Points[I + 1] - Points[I];
		const float Diameter = FMath::Min(Chain->ChainLinkThickness, RopeSimulation->GetCollisionRadius() * 1.8f);
		const float Length = Edge.Size();
		const FQuat Alignment = FRotationMatrix::MakeFromZ(Edge.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector)).ToQuat();
		const FQuat Twist(FVector::UpVector, (I & 1) * UE_HALF_PI);
		const float RadialScale = Diameter / FMath::Max(1.0, FVector2D(MeshSize.X, MeshSize.Y).Size());
		const float SafeOverlap = 2.0f * FMath::Sqrt(FMath::Max(0.0f,
			FMath::Square(RopeSimulation->GetCollisionRadius()) - FMath::Square(Diameter * 0.5f)));
		const FVector Scale(RadialScale, RadialScale,
			(Length + FMath::Min3(Chain->ChainLinkVisualOverlap, RopeSimulation->GetCollisionRadius(), SafeOverlap)) / FMath::Max(1.0, MeshSize.Z));
		const FVector Centre = (Points[I] + Points[I + 1]) * 0.5
			- (Alignment * Twist).RotateVector(MeshBounds.GetCenter() * Scale);
		ChainLinkVisuals->UpdateInstanceTransform(I,
			FTransform(Alignment * Twist, Centre, Scale), true, I == Count - 1, true);
	}
}

void AChopItQuotaMachine::UpdateReleasedChainLabel()
{
	if (!ReleasedChainLabel) return;
	const int32 ReleasedDecimeters = FMath::RoundToInt(CurrentCableLength / 10.0f);
	if (ReleasedDecimeters == LastDisplayedReleasedDecimeters) return;
	LastDisplayedReleasedDecimeters = ReleasedDecimeters;
	ReleasedChainLabel->SetText(FText::FromString(FString::Printf(
		TEXT("CADENA LIBERADA: %.1f m"), static_cast<float>(ReleasedDecimeters) / 10.0f)));
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
	if (ACharacter* Character = Cast<ACharacter>(ChainedPlayer))
	{
		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			for (const TWeakObjectPtr<AActor>& Actor : DeathBypassedActors)
				if (Actor.IsValid()) Capsule->IgnoreActorWhenMoving(Actor.Get(), false);
		}
	}
	DeathBypassedActors.Reset();
	if (TetherReceiver)
	{
		TetherReceiver->BindMachine(nullptr);
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
	CurrentCableLength = 0.0f;
	LastDisplayedReleasedDecimeters = INDEX_NONE;
	bHardLimited = false;
}
