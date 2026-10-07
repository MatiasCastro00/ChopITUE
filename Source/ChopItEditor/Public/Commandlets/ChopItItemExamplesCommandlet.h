#pragma once
#include "Commandlets/Commandlet.h"
#include "ChopItItemExamplesCommandlet.generated.h"

/** Creates missing example assets only; preserves designer edits on reruns. */
UCLASS()
class CHOPITEDITOR_API UChopItItemExamplesCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	virtual int32 Main(const FString& Params) override;
};
