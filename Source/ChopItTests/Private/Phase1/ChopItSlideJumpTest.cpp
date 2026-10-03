#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Player/ChopItCharacter.h"
#include "Tests/AutomationEditorCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItCrouchedMomentumJumpTest,
	"ChopIt.Movement.CrouchedMomentumJump",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItCrouchedMomentumJumpTest::RunTest(const FString& Parameters)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	AChopItCharacter* Character = World->SpawnActor<AChopItCharacter>();
	if (!TestNotNull(TEXT("Character"), Character)) return false;
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	// Editor test worlds do not run the gameplay component initialization pass.
	Movement->SetUpdatedComponent(Character->GetCapsuleComponent());
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	Movement->SetMovementMode(MOVE_Walking);
	Character->Crouch();
	Movement->Crouch(false);
	TestTrue(TEXT("Starts physically crouched, as during a slide"), Movement->IsCrouching());
	const FVector Momentum(900.0f, 150.0f, 0.0f);
	Movement->Velocity = Momentum;

	Character->Jump();
	// Match the engine's ordering: jump input is checked BEFORE deferred posture updates.
	Character->CheckJumpInput(1.0f / 60.0f);
	TestFalse(TEXT("Capsule stands before jump validation"), Movement->IsCrouching());
	TestTrue(TEXT("First press leaves the floor"), Movement->IsFalling());
	TestTrue(TEXT("Jump has upward velocity"), Movement->Velocity.Z > 0.0f);
	TestEqual(TEXT("Jump preserves horizontal X momentum"), Movement->Velocity.X, Momentum.X);
	TestEqual(TEXT("Jump preserves horizontal Y momentum"), Movement->Velocity.Y, Momentum.Y);
	Character->Destroy();
	return true;
}

#endif
