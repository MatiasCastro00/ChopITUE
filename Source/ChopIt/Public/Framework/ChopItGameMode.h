#pragma once

#include "GameFramework/GameModeBase.h"
#include "ChopItGameMode.generated.h"

/** Composition root for a local ChopIt run. */
UCLASS()
class CHOPIT_API AChopItGameMode final : public AGameModeBase
{
	GENERATED_BODY()

public:
	AChopItGameMode();
	virtual void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot) override;
};
