#pragma once

#include "Commandlets/Commandlet.h"
#include "ChopItStickItemDataAssetsCommandlet.generated.h"

/** Creates icon-ready, loot-disabled Data Assets for every supplied stick illustration. */
UCLASS()
class CHOPITEDITOR_API UChopItStickItemDataAssetsCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	virtual int32 Main(const FString& Params) override;
};
