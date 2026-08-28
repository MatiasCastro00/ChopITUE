#include "Economy/ChopItQuotaMachine.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Economy/ChopItQuotaComponent.h"
#include "GameFramework/GameStateBase.h"
#include "UObject/ConstructorHelpers.h"

AChopItQuotaMachine::AChopItQuotaMachine()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MachineVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MachineVisual"));
	MachineVisual->SetupAttachment(SceneRoot);
	MachineVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	MachineVisual->SetRelativeScale3D(FVector(1.2f, 1.2f, 2.0f));
	MachineVisual->SetCollisionProfileName(TEXT("BlockAll"));

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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		MachineVisual->SetStaticMesh(CubeMesh.Object);
	}
}

void AChopItQuotaMachine::BeginPlay()
{
	Super::BeginPlay();
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

void AChopItQuotaMachine::HandlePhaseChanged(
	const EChopItCyclePhase,
	const EChopItCyclePhase,
	const int32)
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
