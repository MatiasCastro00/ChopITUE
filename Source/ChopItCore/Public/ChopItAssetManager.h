#pragma once

#include "Engine/AssetManager.h"
#include "ChopItAssetManager.generated.h"

/** Project asset manager and the single entry point for stable content IDs. */
UCLASS()
class CHOPITCORE_API UChopItAssetManager final : public UAssetManager
{
	GENERATED_BODY()

public:
	static UChopItAssetManager& Get();

protected:
	virtual void StartInitialLoading() override;
};
