#include "Framework/ChopItGameMode.h"

#include "ChopItLogChannels.h"
#include "Framework/ChopItGameState.h"
#include "Framework/ChopItPlayerController.h"
#include "Framework/ChopItPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Player/ChopItCharacter.h"
#include "UI/ChopItHUD.h"
#include "UObject/ConstructorHelpers.h"

AChopItGameMode::AChopItGameMode()
{
	DefaultPawnClass = AChopItCharacter::StaticClass();
	static ConstructorHelpers::FClassFinder<APawn> CharacterBlueprint(
		TEXT("/Game/ChopIt/Characters/Blueprints/BP_ChopItCharacter"));
	if (CharacterBlueprint.Succeeded())
	{
		DefaultPawnClass = CharacterBlueprint.Class;
	}
	PlayerControllerClass = AChopItPlayerController::StaticClass();
	GameStateClass = AChopItGameState::StaticClass();
	PlayerStateClass = AChopItPlayerState::StaticClass();
	HUDClass = AChopItHUD::StaticClass();
}

void AChopItGameMode::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	if (!NewPlayer || !StartSpot || !DefaultPawnClass)
	{
		Super::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
		return;
	}

	float CapsuleHalfHeight = 88.0f;
	if (const ACharacter* CharacterCDO = Cast<ACharacter>(DefaultPawnClass->GetDefaultObject()))
	{
		CapsuleHalfHeight = CharacterCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}

	const FVector StartLocation = StartSpot->GetActorLocation();
	const FVector TraceStart = StartLocation + FVector::UpVector * 5000.0f;
	const FVector TraceEnd = StartLocation - FVector::UpVector * 5000.0f;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ChopItPlayerSpawnTrace), false, StartSpot);
	FHitResult GroundHit;
	if (GetWorld()->LineTraceSingleByObjectType(
		GroundHit,
		TraceStart,
		TraceEnd,
		FCollisionObjectQueryParams(ECC_WorldStatic),
		QueryParams))
	{
		FTransform SpawnTransform = StartSpot->GetActorTransform();
		SpawnTransform.SetLocation(FVector(
			StartLocation.X,
			StartLocation.Y,
			GroundHit.ImpactPoint.Z + CapsuleHalfHeight + 2.0f));
		SpawnTransform.SetScale3D(FVector::OneVector);
		UE_LOG(
			LogChopIt,
			Display,
			TEXT("Player spawn resolved before creation: ground Z=%.2f, capsule half-height=%.2f, spawn Z=%.2f"),
			GroundHit.ImpactPoint.Z,
			CapsuleHalfHeight,
			SpawnTransform.GetLocation().Z);
		RestartPlayerAtTransform(NewPlayer, SpawnTransform);
		return;
	}

	Super::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
}
