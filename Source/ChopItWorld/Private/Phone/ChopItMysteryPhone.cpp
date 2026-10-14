#include "Phone/ChopItMysteryPhone.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Pawn.h"
#include "Phone/ChopItPhoneStateComponent.h"
#include "Phone/ChopItPhoneCallComponent.h"
#include "Phone/ChopItPhoneVariantComponent.h"
#include "Phone/ChopItPhoneRevealComponent.h"
#include "Phone/ChopItPhoneRingMotionComponent.h"
#include "UObject/ConstructorHelpers.h"

AChopItMysteryPhone::AChopItMysteryPhone()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	InteractionBody = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionBody"));
	InteractionBody->SetupAttachment(Root);
	InteractionBody->InitSphereRadius(70.0f);
	InteractionBody->SetRelativeLocation(FVector(0, 0, 70));
	InteractionBody->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBody->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionBody->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBody->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	InteractionBody->SetGenerateOverlapEvents(false);
	HearingRangeGizmo = CreateDefaultSubobject<USphereComponent>(TEXT("HearingRangeGizmo"));
	HearingRangeGizmo->SetupAttachment(Root);
	HearingRangeGizmo->InitSphereRadius(HearingRange);
	HearingRangeGizmo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HearingRangeGizmo->SetGenerateOverlapEvents(false);
	HearingRangeGizmo->SetHiddenInGame(true);
	PromptDetection = CreateDefaultSubobject<USphereComponent>(TEXT("PromptDetection"));
	PromptDetection->SetupAttachment(Root);
	PromptDetection->InitSphereRadius(InteractionRange + 100.0f);
	PromptDetection->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PromptDetection->SetCollisionObjectType(ECC_WorldDynamic);
	PromptDetection->SetCollisionResponseToAllChannels(ECR_Ignore);
	PromptDetection->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PromptDetection->SetGenerateOverlapEvents(true);
	PromptDetection->SetHiddenInGame(true);
	PromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptWidget"));
	PromptWidget->SetupAttachment(Root);
	PromptWidget->SetRelativeLocation(FVector(0, 0, 130));
	PromptWidget->SetWidgetSpace(EWidgetSpace::Screen);
	PromptWidget->SetInitialSharedLayerName(FName(TEXT("PhonePromptLayer")));
	PromptWidget->SetInitialLayerZOrder(-200);
	PromptWidget->SetDrawSize(FVector2D(112, 120));
	PromptWidget->SetPivot(FVector2D(0.5f, 1.0f));
	PromptWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PromptWidget->SetVisibility(false);

	BaseVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseVisual"));
	BaseVisual->SetupAttachment(Root);
	BaseVisual->SetRelativeLocation(FVector(0, 0, 24));
	BaseVisual->SetRelativeScale3D(FVector(0.48f, 0.38f, 0.3f));
	BaseVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ReceiverVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReceiverVisual"));
	ReceiverVisual->SetupAttachment(Root);
	ReceiverVisual->SetRelativeLocation(FVector(0, 0, 48));
	ReceiverVisual->SetRelativeScale3D(FVector(0.6f, 0.14f, 0.12f));
	ReceiverVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Placeholder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PhoneBase(
		TEXT("/Game/ChopIt/Phone/SM_RotaryPhoneBase.SM_RotaryPhoneBase"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PhoneReceiver(
		TEXT("/Game/ChopIt/Phone/SM_RotaryPhoneReceiver.SM_RotaryPhoneReceiver"));
	if (PhoneBase.Succeeded() && PhoneReceiver.Succeeded())
	{
		BaseVisual->SetStaticMesh(PhoneBase.Object);
		ReceiverVisual->SetStaticMesh(PhoneReceiver.Object);
		BaseVisual->SetRelativeScale3D(FVector::OneVector);
		ReceiverVisual->SetRelativeScale3D(FVector::OneVector);
	}
	else if (Placeholder.Succeeded())
	{
		BaseVisual->SetStaticMesh(Placeholder.Object);
		ReceiverVisual->SetStaticMesh(Placeholder.Object);
	}
	PhoneState = CreateDefaultSubobject<UChopItPhoneStateComponent>(TEXT("PhoneState"));
	PhoneCall = CreateDefaultSubobject<UChopItPhoneCallComponent>(TEXT("PhoneCall"));
	PhoneVariant = CreateDefaultSubobject<UChopItPhoneVariantComponent>(TEXT("PhoneVariant"));
	PhoneReveal = CreateDefaultSubobject<UChopItPhoneRevealComponent>(TEXT("PhoneReveal"));
	PhoneRingMotion = CreateDefaultSubobject<UChopItPhoneRingMotionComponent>(TEXT("PhoneRingMotion"));
}

void AChopItMysteryPhone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (HearingRangeGizmo) HearingRangeGizmo->SetSphereRadius(HearingRange, false);
	if (PromptDetection) PromptDetection->SetSphereRadius(InteractionRange + 100.0f, false);
}

bool AChopItMysteryPhone::CanInteract_Implementation(AActor* Interactor) const
{
	if (!IsValid(Interactor) || !Interactor->IsA<APawn>() || !PhoneState) return false;
	const EChopItPhoneState State = PhoneState->GetState();
	return (State == EChopItPhoneState::Available || State == EChopItPhoneState::Ringing)
		&& FVector::DistSquared(GetActorLocation(), Interactor->GetActorLocation()) <= FMath::Square(InteractionRange);
}

bool AChopItMysteryPhone::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor) || !PhoneState->TryTransition(EChopItPhoneState::Answered)) return false;
	OnAnswered(Interactor);
	return true;
}
