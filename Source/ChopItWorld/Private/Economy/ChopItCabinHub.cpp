#include "Economy/ChopItCabinHub.h"

#include "ChopItCollision.h"
#include "Components/SceneComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/GameStateBase.h"
#include "UObject/ConstructorHelpers.h"

AChopItCabinHub::AChopItCabinHub()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CabinVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CabinVisual"));
	CabinVisual->SetupAttachment(SceneRoot);
	CabinVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
	CabinVisual->SetRelativeScale3D(FVector(4.5f, 3.5f, 3.6f));
	CabinVisual->SetCollisionProfileName(TEXT("BlockAll"));
	CabinVisual->SetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid, ECR_Ignore);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		CabinVisual->SetStaticMesh(CubeMesh.Object);
	}

	GuidanceLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GuidanceLight"));
	GuidanceLight->SetupAttachment(SceneRoot);
	GuidanceLight->SetRelativeLocation(FVector(0.0f, 0.0f, 650.0f));
	GuidanceLight->SetLightColor(FLinearColor(1.0f, 0.65f, 0.05f));
	GuidanceLight->SetIntensity(15000.0f);
	GuidanceLight->SetAttenuationRadius(2200.0f);
	GuidanceLight->SetCastShadows(false);

	GuidanceLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("GuidanceLabel"));
	GuidanceLabel->SetupAttachment(SceneRoot);
	GuidanceLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 850.0f));
	GuidanceLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	GuidanceLabel->SetHorizontalAlignment(EHTA_Center);
	GuidanceLabel->SetWorldSize(90.0f);
	GuidanceLabel->SetText(FText::FromString(TEXT("v  CABANA / CUOTA  v")));
	GuidanceLabel->SetTextRenderColor(FColor(255, 190, 30));
	GuidanceLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGuidanceActive(false);
}

void AChopItCabinHub::BeginPlay()
{
	Super::BeginPlay();
	// Existing Blueprint defaults may predate the opt-in camera channel.
	CabinVisual->SetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid, ECR_Ignore);
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (UChopItCycleStateMachineComponent* Cycle = GameState
		? GameState->FindComponentByClass<UChopItCycleStateMachineComponent>() : nullptr)
	{
		Cycle->OnPhaseChanged.AddUniqueDynamic(this, &AChopItCabinHub::HandlePhaseChanged);
		HandlePhaseChanged(Cycle->GetCurrentPhase(), EChopItCyclePhase::Bootstrap, Cycle->GetPhaseGeneration());
	}
}

void AChopItCabinHub::HandlePhaseChanged(
	const EChopItCyclePhase NewPhase,
	const EChopItCyclePhase,
	const int32)
{
	SetGuidanceActive(NewPhase == EChopItCyclePhase::Dusk || NewPhase == EChopItCyclePhase::Night);
}

void AChopItCabinHub::SetGuidanceActive(const bool bActive)
{
	GuidanceLight->SetVisibility(bActive);
	GuidanceLabel->SetVisibility(bActive);
}
