#include "Camera/CameraComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "EnhancedActionKeyMapping.h"
#include "Framework/ChopItGameMode.h"
#include "Framework/ChopItGameState.h"
#include "Framework/ChopItPlayerController.h"
#include "Framework/ChopItPlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Harvest/ChopItTree.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Interaction/ChopItInteractionComponent.h"
#include "Misc/AutomationTest.h"
#include "Player/ChopItCharacter.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase1GameplayFrameworkTest,
	"ChopIt.Phase1.GameplayFramework",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase1GameplayFrameworkTest::RunTest(const FString& Parameters)
{
	const AChopItGameMode* GameMode = GetDefault<AChopItGameMode>();
	TestNotNull(TEXT("Game mode CDO exists"), GameMode);
	if (GameMode)
	{
		TestTrue(TEXT("Default pawn derives from ChopIt character"), GameMode->DefaultPawnClass->IsChildOf(AChopItCharacter::StaticClass()));
		TestTrue(TEXT("Controller class"), GameMode->PlayerControllerClass == AChopItPlayerController::StaticClass());
		TestTrue(TEXT("Game state class"), GameMode->GameStateClass == AChopItGameState::StaticClass());
		TestTrue(TEXT("Player state class"), GameMode->PlayerStateClass == AChopItPlayerState::StaticClass());
	}

	const AChopItCharacter* Character = GetDefault<AChopItCharacter>();
	TestNotNull(TEXT("Character CDO exists"), Character);
	if (Character)
	{
		TestNotNull(TEXT("Top-down camera exists"), Character->GetTopDownCamera());
		TestNotNull(TEXT("Fixed camera boom exists"), Character->GetCameraBoom());
		TestNotNull(TEXT("Interaction seam exists"), Character->GetInteractionComponent());
		TestFalse(TEXT("Character has no actor Tick"), Character->PrimaryActorTick.bCanEverTick);
		TestFalse(TEXT("Character is not snapped into the ground plane"), Character->GetCharacterMovement()->bConstrainToPlane);
		TestEqual(TEXT("Character uses normal gravity"), Character->GetCharacterMovement()->GravityScale, 1.0f);
		TestEqual(TEXT("Character lands in walking mode"), Character->GetCharacterMovement()->DefaultLandMovementMode, MOVE_Walking);
		TestEqual(TEXT("Perspective spike FOV"), Character->GetTopDownCamera()->FieldOfView, 35.0f);
		TestEqual(TEXT("Camera arm length"), Character->GetCameraBoom()->TargetArmLength, 2300.0f);
	}

	TestEqual(TEXT("Unit input remains unit input"), AChopItCharacter::NormalizeMovementInput(FVector2D(0.0, 1.0)), FVector2D(0.0, 1.0));
	const FVector2D Diagonal = AChopItCharacter::NormalizeMovementInput(FVector2D(1.0, 1.0));
	TestTrue(TEXT("Diagonal input is normalized"), FMath::IsNearlyEqual(Diagonal.Size(), 1.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase1InputAssetsTest,
	"ChopIt.Phase1.InputAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase1InputAssetsTest::RunTest(const FString& Parameters)
{
	const UInputAction* Move = LoadObject<UInputAction>(nullptr, TEXT("/Game/ChopIt/Input/IA_Move.IA_Move"));
	const UInputAction* Interact = LoadObject<UInputAction>(nullptr, TEXT("/Game/ChopIt/Input/IA_Interact.IA_Interact"));
	const UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/ChopIt/Input/IMC_Gameplay.IMC_Gameplay"));
	TestNotNull(TEXT("IA_Move exists"), Move);
	TestNotNull(TEXT("IA_Interact exists"), Interact);
	TestNotNull(TEXT("IMC_Gameplay exists"), Context);
	if (!Move || !Interact || !Context)
	{
		return false;
	}

	TestEqual(TEXT("Move is Axis2D"), Move->ValueType, EInputActionValueType::Axis2D);
	TestEqual(TEXT("Interact is Boolean"), Interact->ValueType, EInputActionValueType::Boolean);

	TSet<FKey> MoveKeys;
	TSet<FKey> InteractKeys;
	for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
	{
		if (Mapping.Action == Move)
		{
			MoveKeys.Add(Mapping.Key);
		}
		else if (Mapping.Action == Interact)
		{
			InteractKeys.Add(Mapping.Key);
		}
	}

	for (const FKey Key : { EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::Gamepad_Left2D })
	{
		TestTrue(FString::Printf(TEXT("Move key mapped: %s"), *Key.ToString()), MoveKeys.Contains(Key));
	}
	TestTrue(TEXT("Keyboard interaction mapped"), InteractKeys.Contains(EKeys::E));
	TestTrue(TEXT("Gamepad interaction mapped"), InteractKeys.Contains(EKeys::Gamepad_FaceButton_Bottom));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase1MapTest,
	"ChopIt.Phase1.SandboxMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase1MapTest::RunTest(const FString& Parameters)
{
	UPackage* Package = LoadPackage(nullptr, TEXT("/Game/ChopIt/World/Maps/L_Dev_Sandbox"), LOAD_None);
	TestNotNull(TEXT("Sandbox package loads"), Package);
	if (!Package)
	{
		return false;
	}

	UWorld* World = FindObject<UWorld>(Package, TEXT("L_Dev_Sandbox"));
	TestNotNull(TEXT("Sandbox world exists"), World);
	if (!World)
	{
		return false;
	}

	TestTrue(TEXT("Map overrides ChopIt game mode"), World->GetWorldSettings()->DefaultGameMode == AChopItGameMode::StaticClass());
	int32 PlayerStartCount = 0;
	int32 StaticMeshCount = 0;
	int32 TreeCount = 0;
	for (AActor* Actor : World->PersistentLevel->Actors)
	{
		if (IsValid(Actor))
		{
			PlayerStartCount += Actor->IsA<APlayerStart>() ? 1 : 0;
			StaticMeshCount += Actor->IsA<AStaticMeshActor>() ? 1 : 0;
			TreeCount += Actor->IsA<AChopItTree>() ? 1 : 0;
		}
	}
	TestEqual(TEXT("Exactly one player start"), PlayerStartCount, 1);
	TestTrue(TEXT("Blockout geometry is present"), StaticMeshCount + TreeCount >= 30);
	TestEqual(TEXT("Sandbox has damageable trees"), TreeCount, 20);
	return true;
}

#endif
