#pragma once

#include "GameFramework/GameModeBase.h"
#include "ChopItTerrainTestGameMode.generated.h"

class AChopItTerrainActor;

/** Generates the project recipe before players enter the terrain test world. */
UCLASS()
class CHOPITWORLD_API AChopItTerrainTestGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AChopItTerrainTestGameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Terrain")
	bool bGenerateTerrainAtRuntime = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Terrain")
	int32 RuntimeSeed = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Terrain")
	TObjectPtr<AChopItTerrainActor> GeneratedTerrain;
};
