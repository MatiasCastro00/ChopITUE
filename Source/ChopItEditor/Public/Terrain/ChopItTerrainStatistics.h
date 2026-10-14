#pragma once

#include "CoreMinimal.h"

class AChopItTerrainActor;

/** Snapshot of generated terrain data for the editor status panel. */
class CHOPITEDITOR_API FChopItTerrainStatistics
{
public:
	static FString Describe(const AChopItTerrainActor* Actor);
};
