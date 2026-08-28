#pragma once

#include "CoreMinimal.h"

/** Small synthesized one-shot effects so the blockout has audible feedback before final audio arrives. */
namespace ChopItFeedbackAudio
{
	CHOPITPRESENTATION_API void PlayWoodImpact(UObject* WorldContextObject, const FVector& Location, bool bCritical, float Volume);
	CHOPITPRESENTATION_API void PlayAxeSwing(UObject* WorldContextObject, const FVector& Location, float Volume);
	CHOPITPRESENTATION_API void PlayPhaseSting(UObject* WorldContextObject, const FVector& Location, int32 PhaseIndex, float Volume);
}
