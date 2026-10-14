#pragma once

#include "CoreMinimal.h"

namespace ChopItTerrain
{
	uint32 Mix(uint32 Value);
	double Noise(double X, double Y, uint32 Seed);
	double Fade(double T);
}
