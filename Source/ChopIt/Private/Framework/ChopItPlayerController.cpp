#include "Framework/ChopItPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Framework/ChopItPlayerState.h"
#include "Progression/ChopItUpgradeOfferComponent.h"
#include "Shop/ChopItShopComponent.h"
#include "Pacts/ChopItPactComponent.h"
#include "Player/ChopItCharacter.h"
#include "Framework/ChopItGameState.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Camera/ChopItCameraDirectorSubsystem.h"
#include "GameFramework/GameplayCamerasPlayerCameraManager.h"

AChopItPlayerController::AChopItPlayerController()
{
	PlayerCameraManagerClass = AGameplayCamerasPlayerCameraManager::StaticClass();
}

void AChopItPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			const UInputMappingContext* GameplayContext = LoadObject<UInputMappingContext>(
				nullptr,
				TEXT("/Game/ChopIt/Input/IMC_Gameplay.IMC_Gameplay"));

			if (GameplayContext)
			{
				InputSubsystem->AddMappingContext(GameplayContext, 0);
			}
		}
	}
}

void AChopItPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent)
	{
		return;
	}
	FInputKeyBinding& One = InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AChopItPlayerController::SelectUpgradeOne);
	One.bExecuteWhenPaused = true;
	FInputKeyBinding& Two = InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AChopItPlayerController::SelectUpgradeTwo);
	Two.bExecuteWhenPaused = true;
	FInputKeyBinding& Three = InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AChopItPlayerController::SelectUpgradeThree);
	Three.bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AChopItPlayerController::CloseShop);
}

void AChopItPlayerController::SelectUpgradeOne()
{
	SelectUpgrade(0);
}

void AChopItPlayerController::SelectUpgradeTwo()
{
	SelectUpgrade(1);
}

void AChopItPlayerController::SelectUpgradeThree()
{
	SelectUpgrade(2);
}

void AChopItPlayerController::SelectUpgrade(const int32 Index)
{
	if (ULocalPlayer* LP = GetLocalPlayer()) if (const UChopItCameraDirectorSubsystem* Camera = LP->GetSubsystem<UChopItCameraDirectorSubsystem>(); Camera && Camera->IsInputLocked(EChopItCameraInputLock::Actions)) return;
	if (const AChopItGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChopItGameState>() : nullptr)
	{
		if (UChopItCycleStateMachineComponent* Cycle = GameState->GetCycleStateMachine();
			Cycle && Cycle->GetCurrentPhase() == EChopItCyclePhase::Victory)
		{
			if (Index == 0) { Cycle->RequestVictoryRetirement(); }
			else if (Index == 1) { Cycle->RequestInfiniteMode(); }
			return;
		}
	}
	if (const AChopItPlayerState* State = Cast<AChopItPlayerState>(PlayerState))
	{
		if (UChopItUpgradeOfferComponent* Offers = State->GetUpgradeOfferComponent())
		{
			if (Offers->HasActiveOffer())
			{
				Offers->SelectOffer(Index);
				return;
			}
		}
		if (UChopItShopComponent* Shop = State->GetShopComponent())
		{
			if (UChopItPactComponent* Pacts=State->GetPactComponent(); Pacts && Pacts->HasActiveOffer())
			{
				if (AChopItCharacter* ChopItCharacter=Cast<AChopItCharacter>(GetPawn())) Pacts->SelectOffer(Index,ChopItCharacter->GetCombatStatsComponent());
				return;
			}
			Shop->SelectOffer(Index, GetPawn());
		}
	}
}

void AChopItPlayerController::CloseShop()
{
	if (ULocalPlayer* LP = GetLocalPlayer()) if (const UChopItCameraDirectorSubsystem* Camera = LP->GetSubsystem<UChopItCameraDirectorSubsystem>(); Camera && Camera->IsInputLocked(EChopItCameraInputLock::Actions)) return;
	if (const AChopItPlayerState* State = Cast<AChopItPlayerState>(PlayerState))
	{
		if (UChopItShopComponent* Shop = State->GetShopComponent()) { Shop->CloseShop(); }
	}
}
