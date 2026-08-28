#pragma once
#include "GameFramework/SaveGame.h"
#include "ChopItProfileSaveGame.generated.h"
UCLASS()
class CHOPITMETA_API UChopItProfileSaveGame final : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY() int32 Version=1;
	UPROPERTY() int64 BankedMoney=0;
	UPROPERTY() int32 CompletedRuns=0;
	UPROPERTY() int32 FailedRuns=0;
	UPROPERTY() TArray<FName> UnlockedCharacterIds;
};
