#include "Player/ChopItCharacter.h"

#include "Animation/AnimSequence.h"
#include "Camera/ChopItCameraComponent.h"
#include "ChopItCollision.h"
#include "ChopItLogChannels.h"
#include "Combat/ChopItCombatStatsComponent.h"
#include "Combat/ChopItHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/ChopItCameraFacingTextComponent.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Economy/ChopItEconomyComponent.h"
#include "Economy/ChopItCabinHub.h"
#include "Economy/ChopItQuotaComponent.h"
#include "Economy/ChopItTetherReceiverComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Interaction/ChopItInteractionComponent.h"
#include "Feedback/ChopItHitFeedbackComponent.h"
#include "Feedback/ChopItAttackFeedbackComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Progression/ChopItExperienceComponent.h"
#include "Progression/ChopItUpgradeDefinition.h"
#include "Progression/ChopItUpgradeOfferComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/ChopItAutoAttackComponent.h"
#include "Weapons/ChopItWeaponLoadoutComponent.h"

AChopItCharacter::AChopItCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

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
	Movement->bCanWalkOffLedgesWhenCrouching = false;
	Movement->bConstrainToPlane = false;
	Movement->bSnapToPlaneAtStart = false;
	// A stronger gravity keeps the hop compact and makes landings feel responsive.
	Movement->GravityScale = 1.5f;
	Movement->DefaultLandMovementMode = MOVE_Walking;
	Movement->JumpZVelocity = 600.0f;
	Movement->AirControl = 0.35f;
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	Movement->SetCrouchedHalfHeight(60.0f);

	CameraComponent = CreateDefaultSubobject<UChopItCameraComponent>(TEXT("ChopItCamera"));
	CameraComponent->SetupAttachment(RootComponent);

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

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> LumberjackMesh(
		TEXT("/Game/ChopIt/Art/Character/Updated/c85471db_c635_45fc_bae4_6171d60be24f.c85471db_c635_45fc_bae4_6171d60be24f"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> LumberjackIdle(
		TEXT("/Game/ChopIt/Art/Character/Updated/LumberJackCTRL_COG_Idle_Respiracion_96.LumberJackCTRL_COG_Idle_Respiracion_96"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> LumberjackWalk(
		TEXT("/Game/ChopIt/Art/Character/Updated/LumberJackCTRL_COG_Run_Heavy_650_14.LumberJackCTRL_COG_Run_Heavy_650_14"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> LumberjackJump(
		TEXT("/Game/ChopIt/Art/Character/Updated/LumberJackCTRL_COG_Jump_Quick_24.LumberJackCTRL_COG_Jump_Quick_24"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LumberjackMaterial(
		TEXT("/Game/ChopIt/Art/Character/M_Lumberjack.M_Lumberjack"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LumberjackOutlineMaterial(
		TEXT("/Game/ChopIt/Art/Materials/PSX_Materials/MI_PSX_Outline_Jittering.MI_PSX_Outline_Jittering"));
	if (LumberjackMesh.Succeeded())
	{
		USkeletalMeshComponent* CharacterMesh = GetMesh();
		CharacterMesh->SetSkeletalMeshAsset(LumberjackMesh.Object);
		CharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CharacterMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
		CharacterMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		CharacterMesh->SetRelativeScale3D(FVector(1.65f));
		CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		if (LumberjackMaterial.Succeeded())
		{
			CharacterMesh->SetMaterial(0, LumberjackMaterial.Object);
		}
		if (LumberjackOutlineMaterial.Succeeded())
		{
			CharacterMesh->SetOverlayMaterial(LumberjackOutlineMaterial.Object);
		}
		BodyVisual->SetVisibility(false);
		FacingMarker->SetVisibility(false);
	}
	IdleAnimation = LumberjackIdle.Object;
	WalkAnimation = LumberjackWalk.Object;
	JumpAnimation = LumberjackJump.Object;
	InteractionComponent = CreateDefaultSubobject<UChopItInteractionComponent>(TEXT("InteractionComponent"));
	CombatStatsComponent = CreateDefaultSubobject<UChopItCombatStatsComponent>(TEXT("CombatStatsComponent"));
	HealthComponent = CreateDefaultSubobject<UChopItHealthComponent>(TEXT("HealthComponent"));
	HitFeedbackComponent = CreateDefaultSubobject<UChopItHitFeedbackComponent>(TEXT("HitFeedbackComponent"));
	HitFeedbackComponent->SetVisualComponent(
		LumberjackMesh.Succeeded()
			? static_cast<UPrimitiveComponent*>(GetMesh())
			: static_cast<UPrimitiveComponent*>(BodyVisual.Get()));
	AutoAttackComponent = CreateDefaultSubobject<UChopItAutoAttackComponent>(TEXT("AutoAttackComponent"));
	AttackFeedbackComponent = CreateDefaultSubobject<UChopItAttackFeedbackComponent>(TEXT("AttackFeedbackComponent"));
	WeaponLoadoutComponent = CreateDefaultSubobject<UChopItWeaponLoadoutComponent>(TEXT("WeaponLoadoutComponent"));
	WoodCargoComponent = CreateDefaultSubobject<UChopItWoodCargoComponent>(TEXT("WoodCargoComponent"));
	TetherReceiverComponent = CreateDefaultSubobject<UChopItTetherReceiverComponent>(TEXT("TetherReceiverComponent"));

	WoodCargoLabel = CreateDefaultSubobject<UChopItCameraFacingTextComponent>(TEXT("WoodCargoLabel"));
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

void AChopItCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement && bSlideLandingPending && Movement->IsMovingOnGround())
	{
		bSlideLandingPending = false;
		BeginSlide();
		Crouch();
	}
	if (Movement && bIsSliding)
	{
		if (Movement->IsMovingOnGround())
		{
			if (bCrouchOrSlideHeld && !bPressedJump) Crouch();
			float SlideSpeed = Movement->Velocity.Size2D();
			const FVector FloorNormal = Movement->CurrentFloor.HitResult.ImpactNormal.GetSafeNormal();
			const bool bOnSlope = FloorNormal.Z > 0.0f && FloorNormal.Z < FMath::Cos(FMath::DegreesToRadians(SlideMinSlopeAngle));
			if (!SlideSteeringInput.IsNearlyZero())
			{
				const float CurrentYaw = SlideDirection.Rotation().Yaw;
				const float TargetYaw = SlideSteeringInput.Rotation().Yaw;
				const float SteeredYaw = FMath::FixedTurn(CurrentYaw, TargetYaw, SlideSteeringDegreesPerSecond * DeltaSeconds);
				SlideDirection = FRotator(0.0f, SteeredYaw, 0.0f).Vector();
			}
			const FVector Gravity(0.0f, 0.0f, GetWorld()->GetGravityZ() * Movement->GravityScale);
			const FVector SlopeAcceleration = Gravity - FloorNormal * FVector::DotProduct(Gravity, FloorNormal);
			const float SlopeSpeedChange = FVector::DotProduct(SlopeAcceleration, SlideDirection);
			SlideSpeed = FMath::Clamp(SlideSpeed + (SlopeSpeedChange - 140.0f) * DeltaSeconds, 0.0f, 1250.0f);
			if (SlideSpeed <= 1.0f && !bOnSlope)
			{
				EndSlide();
			}
			else
			{
				Movement->Velocity.X = SlideDirection.X * SlideSpeed;
				Movement->Velocity.Y = SlideDirection.Y * SlideSpeed;
			}
		}
	}
	RefreshCharacterAnimation();
}

void AChopItCharacter::BeginPlay()
{
	Super::BeginPlay();
	// Blueprint component templates can restore their serialized material overrides
	// after the native constructor. Apply the authored character material once all
	// Blueprint defaults have been loaded.
	if (UMaterialInterface* LumberjackMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/ChopIt/Art/Character/M_Lumberjack.M_Lumberjack")))
	{
		GetMesh()->SetMaterial(0, LumberjackMaterial);
	}
	if (UMaterialInterface* LumberjackOutlineMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/ChopIt/Art/Materials/PSX_Materials/MI_PSX_Outline_Jittering.MI_PSX_Outline_Jittering")))
	{
		LumberjackOutlineInstance = UMaterialInstanceDynamic::Create(LumberjackOutlineMaterial, this);
		if (LumberjackOutlineInstance)
		{
			LumberjackOutlineInstance->SetScalarParameterValue(TEXT("OutlineThiknes"), 0.35f);
			GetMesh()->SetOverlayMaterial(LumberjackOutlineInstance);
		}
	}

	// Enforce normal CharacterMovement behavior even if an older Blueprint CDO
	// was reinstanced by Live Coding. Spawn placement belongs to the GameMode.
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	WoodCargoLabel->SetVisibility(false, true);
	WoodCargoLabel->SetHiddenInGame(true, true);
	Movement->SetPlaneConstraintEnabled(false);
	Movement->bSnapToPlaneAtStart = false;
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	Movement->SetCrouchedHalfHeight(60.0f);
	Movement->GravityScale = 1.5f;
	Movement->SetMovementMode(MOVE_Walking);
	StandingMeshScale = GetMesh()->GetRelativeScale3D();
	CombatStatsComponent->OnStatsChanged.AddUObject(this, &AChopItCharacter::RefreshMovementStats);
	HealthComponent->OnDeath.AddUObject(this, &AChopItCharacter::HandlePlayerDeath);
	RefreshMovementStats();
	RefreshCharacterAnimation();
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

void AChopItCharacter::RefreshCharacterAnimation()
{
	if (!GetMesh() || !GetMesh()->GetSkeletalMeshAsset())
	{
		return;
	}

	UAnimSequence* DesiredAnimation = IdleAnimation;
	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		DesiredAnimation = JumpAnimation;
	}
	else if (GetVelocity().SizeSquared2D() > FMath::Square(10.0f))
	{
		DesiredAnimation = WalkAnimation;
	}

	if (DesiredAnimation && DesiredAnimation != ActiveAnimation)
	{
		const bool bLoop = DesiredAnimation != JumpAnimation;
		GetMesh()->PlayAnimation(DesiredAnimation, bLoop);
		ActiveAnimation = DesiredAnimation;
	}
}

void AChopItCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (JumpMappingContext)
	{
		if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
				{
					InputSubsystem->RemoveMappingContext(JumpMappingContext);
				}
			}
		}
	}
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
	if (!JumpMappingContext)
	{
		JumpAction = NewObject<UInputAction>(this);
		JumpAction->ValueType = EInputActionValueType::Boolean;
		JumpMappingContext = NewObject<UInputMappingContext>(this);
		JumpMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
		CrouchOrSlideAction = NewObject<UInputAction>(this);
		CrouchOrSlideAction->ValueType = EInputActionValueType::Boolean;
		JumpMappingContext->MapKey(CrouchOrSlideAction, EKeys::LeftShift);
	}
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				InputSubsystem->AddMappingContext(JumpMappingContext, 0);
			}
		}
	}
	InteractAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/ChopIt/Input/IA_Interact.IA_Interact"));
	CameraLookAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/ChopIt/Input/IA_CameraLook.IA_CameraLook"));
	CameraZoomAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/ChopIt/Input/IA_CameraZoom.IA_CameraZoom"));
	CameraResetAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/ChopIt/Input/IA_CameraReset.IA_CameraReset"));

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AChopItCharacter::HandleMove);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &AChopItCharacter::HandleStopMove);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AChopItCharacter::HandleStopMove);
	}
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AChopItCharacter::HandleJump);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &AChopItCharacter::HandleStopJumping);
	EnhancedInput->BindAction(CrouchOrSlideAction, ETriggerEvent::Started, this, &AChopItCharacter::HandleCrouchOrSlidePressed);
	EnhancedInput->BindAction(CrouchOrSlideAction, ETriggerEvent::Completed, this, &AChopItCharacter::HandleCrouchOrSlideReleased);
	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AChopItCharacter::HandleInteract);
	}
	if (CameraLookAction) EnhancedInput->BindAction(CameraLookAction, ETriggerEvent::Triggered, this, &AChopItCharacter::HandleCameraLook);
	if (CameraZoomAction) EnhancedInput->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &AChopItCharacter::HandleCameraZoom);
	if (CameraResetAction) EnhancedInput->BindAction(CameraResetAction, ETriggerEvent::Started, this, &AChopItCharacter::HandleCameraReset);
}

void AChopItCharacter::Jump()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement) return;

	// CharacterMovement checks jump input before processing deferred uncrouch requests.
	// Complete the collision-checked expansion now so this press can actually jump.
	UnCrouch();
	if (Movement->IsCrouching())
	{
		Movement->UnCrouch(false);
		if (Movement->IsCrouching()) return; // A low ceiling prevents standing up.
	}
	Super::Jump();
}

void AChopItCharacter::HandleJump()
{
	if (!CameraComponent || !CameraComponent->IsInputLocked(EChopItCameraInputLock::Actions))
	{
		Jump();
	}
}

void AChopItCharacter::HandleStopJumping()
{
	StopJumping();
}

void AChopItCharacter::HandleCrouchOrSlidePressed()
{
	if (CameraComponent && CameraComponent->IsInputLocked(EChopItCameraInputLock::Actions))
	{
		return;
	}
	bCrouchOrSlideHeld = true;
	BeginSlide();
	Crouch();
}

void AChopItCharacter::BeginSlide()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || bIsSliding)
	{
		return;
	}
	if (Movement->IsFalling())
	{
		// Prepare low-friction landing, but evaluate entry and apply its boost on the ground.
		bSlideLandingPending = true;
		Movement->GroundFriction = 0.1f;
		Movement->BrakingDecelerationWalking = 0.0f;
		Movement->MaxWalkSpeed = FMath::Max(BaseMaxWalkSpeed, Movement->Velocity.Size2D());
		Movement->MaxWalkSpeedCrouched = Movement->MaxWalkSpeed;
		return;
	}
	if (!Movement->IsMovingOnGround()) return;
	const FVector FloorNormal = Movement->CurrentFloor.HitResult.ImpactNormal.GetSafeNormal();
	const bool bOnSlope = FloorNormal.Z > 0.0f && FloorNormal.Z < FMath::Cos(FMath::DegreesToRadians(SlideMinSlopeAngle));
	const float EntrySpeed = Movement->Velocity.Size2D();
	if (!bOnSlope && EntrySpeed <= SlideMinStartSpeed)
	{
		EndSlide();
		return;
	}
	bIsSliding = true;
	bSlideLandingPending = false;
	Movement->GroundFriction = 0.1f;
	Movement->BrakingDecelerationWalking = 0.0f;
	Movement->MaxWalkSpeed = FMath::Max(BaseMaxWalkSpeed, 1250.0f);
	Movement->MaxWalkSpeedCrouched = 1250.0f;
	Movement->bOrientRotationToMovement = false;
	// The horizontal part of the upward floor normal points downhill.
	SlideDirection = bOnSlope ? FloorNormal.GetSafeNormal2D() : GetActorForwardVector().GetSafeNormal2D();
	const float SlideSpeed = FMath::Min(EntrySpeed * SlideEntryMultiplier, 1250.0f);
	Movement->Velocity.X = SlideDirection.X * SlideSpeed;
	Movement->Velocity.Y = SlideDirection.Y * SlideSpeed;
	ConsumeMovementInputVector();
	if (GetMesh())
	{
		FVector SlideScale = StandingMeshScale;
		SlideScale.Z *= 0.68f;
		GetMesh()->SetRelativeScale3D(SlideScale);
	}
}

void AChopItCharacter::EndSlide()
{
	bIsSliding = false;
	bSlideLandingPending = false;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->GroundFriction = BaseGroundFriction;
		Movement->BrakingDecelerationWalking = BaseBrakingDecelerationWalking;
		Movement->MaxWalkSpeedCrouched = 300.0f;
		Movement->bOrientRotationToMovement = true;
		Movement->MaxWalkSpeed = FMath::Max(BaseMaxWalkSpeed, Movement->Velocity.Size2D());
	}
	if (GetMesh())
	{
		GetMesh()->SetRelativeScale3D(StandingMeshScale);
	}
}

void AChopItCharacter::HandleCrouchOrSlideReleased()
{
	bCrouchOrSlideHeld = false;
	EndSlide();
	UnCrouch();
}

void AChopItCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MovementInput = NormalizeMovementInput(Value.Get<FVector2D>());
	if (CameraComponent && CameraComponent->IsInputLocked(EChopItCameraInputLock::Movement))
	{
		SlideSteeringInput = FVector::ZeroVector;
		return;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	const FRotator CameraRotation = PlayerController && PlayerController->PlayerCameraManager
		? PlayerController->PlayerCameraManager->GetCameraRotation()
		: FRotator(0.0f, CameraComponent ? CameraComponent->GetGameplayView().Yaw : GetActorRotation().Yaw, 0.0f);
	FVector CameraForward = FRotationMatrix(FRotator(0.0f, CameraRotation.Yaw, 0.0f)).GetUnitAxis(EAxis::X);
	CameraForward.Z = 0.0f;
	CameraForward.Normalize();

	FVector CameraRight = FRotationMatrix(FRotator(0.0f, CameraRotation.Yaw, 0.0f)).GetUnitAxis(EAxis::Y);
	CameraRight.Z = 0.0f;
	CameraRight.Normalize();

	FVector WorldDirection = CameraForward * MovementInput.Y + CameraRight * MovementInput.X;
	if (TetherReceiverComponent)
	{
		WorldDirection = TetherReceiverComponent->ConstrainMovementDirection(WorldDirection);
	}
	SlideSteeringInput = WorldDirection.GetSafeNormal2D();
	if (bIsSliding || bSlideLandingPending)
	{
		if (!SlideSteeringInput.IsNearlyZero())
		{
			SetActorRotation(FRotator(0.0f, SlideSteeringInput.Rotation().Yaw, 0.0f));
		}
		return;
	}
	AddMovementInput(WorldDirection);
}

void AChopItCharacter::HandleStopMove()
{
	SlideSteeringInput = FVector::ZeroVector;
}

FVector2D AChopItCharacter::NormalizeMovementInput(const FVector2D& Input)
{
	return Input.GetClampedToMaxSize(1.0f);
}

void AChopItCharacter::HandleInteract(const FInputActionValue& Value)
{
	if (Value.Get<bool>() && InteractionComponent && (!CameraComponent || !CameraComponent->IsInputLocked(EChopItCameraInputLock::Actions)))
	{
		InteractionComponent->TryInteract();
	}
}

void AChopItCharacter::HandleCameraLook(const FInputActionValue& Value)
{
	if (!CameraComponent) return;
	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->GetInputMouseDelta(MouseX, MouseY);
	}
	const FVector2D MouseDelta(MouseX, MouseY);
	if (!MouseDelta.IsNearlyZero()) CameraComponent->AddMouseLookInput(MouseDelta, DeltaSeconds);
	else CameraComponent->AddGamepadLookInput(Value.Get<FVector2D>(), DeltaSeconds);
}

void AChopItCharacter::HandleCameraZoom(const FInputActionValue& Value)
{
	if (CameraComponent) CameraComponent->AddZoomInput(Value.Get<float>());
}

void AChopItCharacter::HandleCameraReset(const FInputActionValue& Value)
{
	if (CameraComponent && Value.Get<bool>()) CameraComponent->ResetGameplayCamera();
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
	BaseMaxWalkSpeed = CombatStatsComponent
		? CombatStatsComponent->EvaluateStat(EChopItCombatStat::MovementSpeed, BaseWalkSpeed)
		: BaseWalkSpeed;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = bIsSliding ? FMath::Max(BaseMaxWalkSpeed, 1250.0f) : FMath::Max(BaseMaxWalkSpeed, Movement->Velocity.Size2D() > BaseMaxWalkSpeed ? Movement->Velocity.Size2D() : BaseMaxWalkSpeed);
	}
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
		FString OfferText = FString::Printf(TEXT("LEVEL %d REACHED - CHOOSE AN UPGRADE\n"), Level);
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
	const TCHAR* PhaseName = TEXT("STARTING");
	FString Guidance;
	switch (Phase)
	{
	case EChopItCyclePhase::Day: PhaseName = TEXT("DAY"); break;
	case EChopItCyclePhase::Dusk: PhaseName = TEXT("DUSK"); break;
	case EChopItCyclePhase::Night: PhaseName = TEXT("NIGHT"); break;
	case EChopItCyclePhase::Elite: PhaseName = TEXT("ELITE INCOMING"); break;
	case EChopItCyclePhase::Resolution: PhaseName = TEXT("CYCLE COMPLETE"); break;
	case EChopItCyclePhase::Death: PhaseName = TEXT("DEFEAT"); break;
	case EChopItCyclePhase::Victory: PhaseName = TEXT("VICTORY: FOREST DEFEATED"); break;
	default: break;
	}
	if ((Phase == EChopItCyclePhase::Dusk || Phase == EChopItCyclePhase::Night) && IsValid(CabinHub))
	{
		FVector ToCabin = CabinHub->GetActorLocation() - GetActorLocation();
		ToCabin.Z = 0.0f;
		const float DistanceMeters = ToCabin.Size() / 100.0f;
		ToCabin.Normalize();
		const float CameraYaw = CameraComponent ? CameraComponent->GetGameplayView().Yaw : GetActorRotation().Yaw;
		FVector CameraForward = FRotationMatrix(FRotator(0.0f, CameraYaw, 0.0f)).GetUnitAxis(EAxis::X);
		CameraForward.Z = 0.0f;
		CameraForward.Normalize();
		FVector CameraRight = FRotationMatrix(FRotator(0.0f, CameraYaw, 0.0f)).GetUnitAxis(EAxis::Y);
		CameraRight.Z = 0.0f;
		CameraRight.Normalize();
		const float ForwardDot = FVector::DotProduct(ToCabin, CameraForward);
		const float RightDot = FVector::DotProduct(ToCabin, CameraRight);
		const TCHAR* Arrow = FMath::Abs(ForwardDot) >= FMath::Abs(RightDot)
			? (ForwardDot >= 0.0f ? TEXT("^") : TEXT("v"))
			: (RightDot >= 0.0f ? TEXT(">") : TEXT("<"));
		Guidance = Phase == EChopItCyclePhase::Night && bInfiniteMode
			? TEXT("\nENDLESS NIGHT: SURVIVE")
			: Phase == EChopItCyclePhase::Night
			? FString::Printf(TEXT("\nELITE ARRIVES IN %.0fs"), FMath::Max(0.0f, Remaining))
			: (DistanceMeters <= 4.0f
				? TEXT("\nCABIN: ARRIVED")
				: FString::Printf(TEXT("\nCABIN / LEVER: %.0fm [%s]"), DistanceMeters, Arrow));
	}
	const FString ClockText = Remaining >= 0.0f ? FString::Printf(TEXT("  %.0fs"), Remaining) : FString();
	WoodCargoLabel->SetText(FText::FromString(FString::Printf(
		TEXT("%s%s%s\nLevel %d  XP %d / %d\nWood %d / %d\nQuota %d / %d\nMoney $%lld"),
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
