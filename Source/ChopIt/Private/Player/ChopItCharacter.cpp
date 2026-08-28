#include "Player/ChopItCharacter.h"

#include "Camera/CameraComponent.h"
#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "Combat/ChopItCombatStatsComponent.h"
#include "Combat/ChopItHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "EnhancedInputComponent.h"
#include "Economy/ChopItEconomyComponent.h"
#include "Economy/ChopItCabinHub.h"
#include "Economy/ChopItQuotaComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "InputAction.h"
#include "Interaction/ChopItInteractionComponent.h"
#include "Feedback/ChopItHitFeedbackComponent.h"
#include "Feedback/ChopItAttackFeedbackComponent.h"
#include "Materials/MaterialInterface.h"
#include "Progression/ChopItExperienceComponent.h"
#include "Progression/ChopItUpgradeDefinition.h"
#include "Progression/ChopItUpgradeOfferComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/ChopItAutoAttackComponent.h"
#include "Weapons/ChopItWeaponLoadoutComponent.h"

AChopItCharacter::AChopItCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0f);
	GetCapsuleComponent()->SetCollisionProfileName(ChopItCollisionProfiles::Player);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	Movement->MaxWalkSpeed = 650.0f;
	Movement->BrakingDecelerationWalking = 2200.0f;
	Movement->GroundFriction = 8.0f;
	Movement->bConstrainToPlane = false;
	Movement->bSnapToPlaneAtStart = false;
	Movement->GravityScale = 1.0f;
	Movement->DefaultLandMovementMode = MOVE_Walking;
	Movement->JumpZVelocity = 600.0f;
	Movement->AirControl = 0.35f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 2300.0f;
	CameraBoom->SetRelativeRotation(FRotator(-58.0f, -45.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.0f;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;
	TopDownCamera->FieldOfView = 35.0f;

	BodyVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyVisual"));
	BodyVisual->SetupAttachment(GetCapsuleComponent());
	BodyVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyVisual->SetRelativeLocation(FVector(0.0f, 0.0f, -5.0f));
	BodyVisual->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.9f));

	FacingMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingMarker"));
	FacingMarker->SetupAttachment(GetCapsuleComponent());
	FacingMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FacingMarker->SetRelativeLocation(FVector(55.0f, 0.0f, 0.0f));
	FacingMarker->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	FacingMarker->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.35f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (CylinderMesh.Succeeded())
	{
		BodyVisual->SetStaticMesh(CylinderMesh.Object);
	}
	if (ConeMesh.Succeeded())
	{
		FacingMarker->SetStaticMesh(ConeMesh.Object);
	}
	InteractionComponent = CreateDefaultSubobject<UChopItInteractionComponent>(TEXT("InteractionComponent"));
	CombatStatsComponent = CreateDefaultSubobject<UChopItCombatStatsComponent>(TEXT("CombatStatsComponent"));
	HealthComponent = CreateDefaultSubobject<UChopItHealthComponent>(TEXT("HealthComponent"));
	HitFeedbackComponent = CreateDefaultSubobject<UChopItHitFeedbackComponent>(TEXT("HitFeedbackComponent"));
	HitFeedbackComponent->SetVisualComponent(BodyVisual);
	AutoAttackComponent = CreateDefaultSubobject<UChopItAutoAttackComponent>(TEXT("AutoAttackComponent"));
	AttackFeedbackComponent = CreateDefaultSubobject<UChopItAttackFeedbackComponent>(TEXT("AttackFeedbackComponent"));
	WeaponLoadoutComponent = CreateDefaultSubobject<UChopItWeaponLoadoutComponent>(TEXT("WeaponLoadoutComponent"));
	WoodCargoComponent = CreateDefaultSubobject<UChopItWoodCargoComponent>(TEXT("WoodCargoComponent"));

	WoodCargoLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("WoodCargoLabel"));
	WoodCargoLabel->SetupAttachment(GetCapsuleComponent());
	WoodCargoLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 145.0f));
	WoodCargoLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	WoodCargoLabel->SetHorizontalAlignment(EHTA_Center);
	WoodCargoLabel->SetWorldSize(26.0f);
	WoodCargoLabel->SetTextRenderColor(FColor::Yellow);
	WoodCargoLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WoodCargoLabel->SetVisibility(false);
	WoodCargoLabel->SetHiddenInGame(true);
}

void AChopItCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Enforce normal CharacterMovement behavior even if an older Blueprint CDO
	// was reinstanced by Live Coding. Spawn placement belongs to the GameMode.
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	WoodCargoLabel->SetVisibility(false, true);
	WoodCargoLabel->SetHiddenInGame(true, true);
	Movement->SetPlaneConstraintEnabled(false);
	Movement->bSnapToPlaneAtStart = false;
	Movement->GravityScale = 1.0f;
	Movement->SetMovementMode(MOVE_Walking);
	CombatStatsComponent->OnStatsChanged.AddUObject(this, &AChopItCharacter::RefreshMovementStats);
	HealthComponent->OnDeath.AddUObject(this, &AChopItCharacter::HandlePlayerDeath);
	RefreshMovementStats();
	WoodCargoComponent->OnCargoChanged.AddUniqueDynamic(this, &AChopItCharacter::HandleWoodCargoChanged);
	for (TActorIterator<AChopItCabinHub> It(GetWorld()); It; ++It)
	{
		CabinHub = *It;
		break;
	}
	if (AGameStateBase* GameState = GetWorld()->GetGameState())
	{
		if (UChopItQuotaComponent* Quota = GameState->FindComponentByClass<UChopItQuotaComponent>())
		{
			Quota->OnQuotaChanged.AddUniqueDynamic(this, &AChopItCharacter::HandleQuotaChanged);
		}
		if (UChopItCycleStateMachineComponent* Cycle = GameState->FindComponentByClass<UChopItCycleStateMachineComponent>())
		{
			Cycle->OnPhaseChanged.AddUniqueDynamic(this, &AChopItCharacter::HandleCyclePhaseChanged);
			Cycle->OnClockChanged.AddUniqueDynamic(this, &AChopItCharacter::HandleCycleClockChanged);
		}
	}
	if (APlayerState* State = GetPlayerState())
	{
		if (UChopItEconomyComponent* Economy = State->FindComponentByClass<UChopItEconomyComponent>())
		{
			Economy->OnBalanceChanged.AddUniqueDynamic(this, &AChopItCharacter::HandleBalanceChanged);
		}
		if (UChopItExperienceComponent* Experience = State->FindComponentByClass<UChopItExperienceComponent>())
		{
			Experience->OnExperienceChanged.AddUniqueDynamic(this, &AChopItCharacter::HandleExperienceChanged);
		}
		if (UChopItUpgradeOfferComponent* Offers = State->FindComponentByClass<UChopItUpgradeOfferComponent>())
		{
			Offers->OnOffersChanged.AddUniqueDynamic(this, &AChopItCharacter::HandleOffersChanged);
		}
	}
	RefreshEconomyDebugLabel();
	UE_LOG(
		LogChopIt,
		Display,
		TEXT("Character BeginPlay: location=%s, movement mode=%d, gravity scale=%.2f"),
		*GetActorLocation().ToCompactString(),
		static_cast<int32>(Movement->MovementMode),
		Movement->GravityScale);

	if (UMaterialInterface* PlayerMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/ChopIt/World/Blockout/Materials/MI_Player.MI_Player")))
	{
		BodyVisual->SetMaterial(0, PlayerMaterial);
		FacingMarker->SetMaterial(0, PlayerMaterial);
	}
}

void AChopItCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CombatStatsComponent)
	{
		CombatStatsComponent->OnStatsChanged.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AChopItCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	MoveAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/ChopIt/Input/IA_Move.IA_Move"));
	InteractAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/ChopIt/Input/IA_Interact.IA_Interact"));

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AChopItCharacter::HandleMove);
	}
	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AChopItCharacter::HandleInteract);
	}
}

void AChopItCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MovementInput = NormalizeMovementInput(Value.Get<FVector2D>());

	FVector CameraForward = TopDownCamera->GetForwardVector();
	CameraForward.Z = 0.0f;
	CameraForward.Normalize();

	FVector CameraRight = TopDownCamera->GetRightVector();
	CameraRight.Z = 0.0f;
	CameraRight.Normalize();

	AddMovementInput(CameraForward, MovementInput.Y);
	AddMovementInput(CameraRight, MovementInput.X);
}

FVector2D AChopItCharacter::NormalizeMovementInput(const FVector2D& Input)
{
	return Input.GetClampedToMaxSize(1.0f);
}

void AChopItCharacter::HandleInteract(const FInputActionValue& Value)
{
	if (Value.Get<bool>() && InteractionComponent)
	{
		InteractionComponent->TryInteract();
	}
}

void AChopItCharacter::HandleWoodCargoChanged(const int32 CurrentWood, const int32 Capacity)
{
	RefreshEconomyDebugLabel();
	UE_LOG(LogChopIt, Display, TEXT("Wood cargo: %d / %d."), CurrentWood, Capacity);
}

void AChopItCharacter::HandleQuotaChanged(const int32, const int32, const bool)
{
	RefreshEconomyDebugLabel();
}

void AChopItCharacter::HandleBalanceChanged(const int64, const int64)
{
	RefreshEconomyDebugLabel();
}

void AChopItCharacter::HandleCyclePhaseChanged(
	const EChopItCyclePhase,
	const EChopItCyclePhase,
	const int32)
{
	RefreshEconomyDebugLabel();
}

void AChopItCharacter::HandleCycleClockChanged(const EChopItCyclePhase, const float)
{
	RefreshEconomyDebugLabel();
}

void AChopItCharacter::HandleExperienceChanged(const int32, const int32, const int32, const int32)
{
	RefreshEconomyDebugLabel();
}

void AChopItCharacter::HandleOffersChanged()
{
	RefreshEconomyDebugLabel();
}

void AChopItCharacter::HandlePlayerDeath(AActor* DeadActor, AActor* DamageSource)
{
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	if (AutoAttackComponent)
	{
		AutoAttackComponent->Deactivate();
	}
	DisableInput(Cast<APlayerController>(GetController()));
	SetActorEnableCollision(false);
	if (AGameStateBase* GameState = GetWorld()->GetGameState())
	{
		if (UChopItCycleStateMachineComponent* Cycle = GameState->FindComponentByClass<UChopItCycleStateMachineComponent>())
		{
			Cycle->RequestDeath(DamageSource);
		}
	}
}

void AChopItCharacter::RefreshMovementStats()
{
	GetCharacterMovement()->MaxWalkSpeed = CombatStatsComponent
		? CombatStatsComponent->EvaluateStat(EChopItCombatStat::MovementSpeed, BaseWalkSpeed)
		: BaseWalkSpeed;
}

void AChopItCharacter::RefreshEconomyDebugLabel()
{
	int32 QuotaProgress = 0;
	int32 QuotaTarget = 0;
	int64 Balance = 0;
	int32 Level = 1;
	int32 ExperienceValue = 0;
	int32 RequiredExperience = 1;
	const UChopItUpgradeOfferComponent* UpgradeOffers = nullptr;
	EChopItCyclePhase Phase = EChopItCyclePhase::Bootstrap;
	float Remaining = -1.0f;
	bool bInfiniteMode = false;
	if (UWorld* World = GetWorld())
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			if (const UChopItQuotaComponent* Quota = GameState->FindComponentByClass<UChopItQuotaComponent>())
			{
				QuotaProgress = Quota->GetProgress();
				QuotaTarget = Quota->GetTarget();
			}
			if (const UChopItCycleStateMachineComponent* Cycle = GameState->FindComponentByClass<UChopItCycleStateMachineComponent>())
			{
				Phase = Cycle->GetCurrentPhase();
				Remaining = Cycle->GetPhaseRemaining();
				bInfiniteMode = Cycle->IsInfiniteMode();
			}
		}
	}
	if (const APlayerState* State = GetPlayerState())
	{
		if (const UChopItEconomyComponent* Economy = State->FindComponentByClass<UChopItEconomyComponent>())
		{
			Balance = Economy->GetBalance();
		}
		if (const UChopItExperienceComponent* Experience = State->FindComponentByClass<UChopItExperienceComponent>())
		{
			Level = Experience->GetLevel();
			ExperienceValue = Experience->GetCurrentExperience();
			RequiredExperience = Experience->GetRequiredExperience();
		}
		UpgradeOffers = State->FindComponentByClass<UChopItUpgradeOfferComponent>();
	}
	if (UpgradeOffers && UpgradeOffers->HasActiveOffer())
	{
		FString OfferText = FString::Printf(TEXT("SUBISTE A NIVEL %d - ELIGE MEJORA\n"), Level);
		const TArray<TObjectPtr<UChopItUpgradeDefinition>>& Offers = UpgradeOffers->GetActiveOffers();
		for (int32 Index = 0; Index < Offers.Num(); ++Index)
		{
			OfferText += FString::Printf(
				TEXT("[%d] %s - %s\n"),
				Index + 1,
				*Offers[Index]->DisplayName.ToString(),
				*Offers[Index]->Description.ToString());
		}
		WoodCargoLabel->SetText(FText::FromString(OfferText));
		WoodCargoLabel->SetTextRenderColor(FColor::Cyan);
		return;
	}
	WoodCargoLabel->SetTextRenderColor(FColor::Yellow);
	const TCHAR* PhaseName = TEXT("INICIO");
	FString Guidance;
	switch (Phase)
	{
	case EChopItCyclePhase::Day: PhaseName = TEXT("DIA"); break;
	case EChopItCyclePhase::Dusk: PhaseName = TEXT("ATARDECER"); break;
	case EChopItCyclePhase::Night: PhaseName = TEXT("NOCHE"); break;
	case EChopItCyclePhase::Elite: PhaseName = TEXT("ELITE PROVISIONAL"); break;
	case EChopItCyclePhase::Resolution: PhaseName = TEXT("CICLO COMPLETO"); break;
	case EChopItCyclePhase::Death: PhaseName = TEXT("DERROTA"); break;
	case EChopItCyclePhase::Victory: PhaseName = TEXT("VICTORIA: BOSQUE VENCIDO"); break;
	default: break;
	}
	if ((Phase == EChopItCyclePhase::Dusk || Phase == EChopItCyclePhase::Night) && IsValid(CabinHub))
	{
		FVector ToCabin = CabinHub->GetActorLocation() - GetActorLocation();
		ToCabin.Z = 0.0f;
		const float DistanceMeters = ToCabin.Size() / 100.0f;
		ToCabin.Normalize();
		FVector CameraForward = TopDownCamera->GetForwardVector();
		CameraForward.Z = 0.0f;
		CameraForward.Normalize();
		FVector CameraRight = TopDownCamera->GetRightVector();
		CameraRight.Z = 0.0f;
		CameraRight.Normalize();
		const float ForwardDot = FVector::DotProduct(ToCabin, CameraForward);
		const float RightDot = FVector::DotProduct(ToCabin, CameraRight);
		const TCHAR* Arrow = FMath::Abs(ForwardDot) >= FMath::Abs(RightDot)
			? (ForwardDot >= 0.0f ? TEXT("^") : TEXT("v"))
			: (RightDot >= 0.0f ? TEXT(">") : TEXT("<"));
		Guidance = Phase == EChopItCyclePhase::Night && bInfiniteMode
			? TEXT("\nNOCHE INFINITA: SOBREVIVI")
			: Phase == EChopItCyclePhase::Night
			? FString::Printf(TEXT("\nELITE APARECE EN %.0fs"), FMath::Max(0.0f, Remaining))
			: (DistanceMeters <= 4.0f
				? TEXT("\nCABANA: LLEGASTE")
				: FString::Printf(TEXT("\nCABANA / PALANCA: %.0fm [%s]"), DistanceMeters, Arrow));
	}
	const FString ClockText = Remaining >= 0.0f ? FString::Printf(TEXT("  %.0fs"), Remaining) : FString();
	WoodCargoLabel->SetText(FText::FromString(FString::Printf(
		TEXT("%s%s%s\nNivel %d  XP %d / %d\nMadera %d / %d\nCuota %d / %d\nDinero $%lld"),
		PhaseName,
		*ClockText,
		*Guidance,
		Level,
		ExperienceValue,
		RequiredExperience,
		WoodCargoComponent->GetCurrentWood(),
		WoodCargoComponent->GetCapacity(),
		QuotaProgress,
		QuotaTarget,
		Balance)));
}
