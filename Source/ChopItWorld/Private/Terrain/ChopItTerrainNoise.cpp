#include "Terrain/ChopItTerrainNoise.h"

namespace ChopItTerrain
{
	// Fixed integer hash, independent of Unreal's global RNG and traversal order.
	uint32 Mix(uint32 Value)
	{
		Value ^= Value >> 16;
		Value *= 0x7feb352du;
		Value ^= Value >> 15;
		Value *= 0x846ca68bu;
		return Value ^ (Value >> 16);
	}

	uint32 Hash(int64 X, int64 Y, uint32 Seed)
	{
		const uint64 UX = uint64(X);
		const uint64 UY = uint64(Y);
		return Mix(Seed ^ Mix(uint32(UX) ^ Mix(uint32(UX >> 32))) ^ Mix(uint32(UY) ^ Mix(uint32(UY >> 32)) ^ 0x9e3779b9u));
	}

	double Fade(double T) { return T * T * T * (T * (T * 6.0 - 15.0) + 10.0); }

	double Gradient(uint32 H, double X, double Y)
	{
		constexpr double Diagonal = 0.7071067811865475;
		switch (H & 7u)
		{
		case 0: return X;
		case 1: return -X;
		case 2: return Y;
		case 3: return -Y;
		case 4: return (X + Y) * Diagonal;
		case 5: return (X - Y) * Diagonal;
		case 6: return (-X + Y) * Diagonal;
		default: return (-X - Y) * Diagonal;
		}
	}

	double Noise(double X, double Y, uint32 Seed)
	{
		const int64 IX = FMath::FloorToInt64(X);
		const int64 IY = FMath::FloorToInt64(Y);
		const double FX = X - double(IX);
		const double FY = Y - double(IY);
		const double A = FMath::Lerp(Gradient(Hash(IX, IY, Seed), FX, FY), Gradient(Hash(IX + 1, IY, Seed), FX - 1.0, FY), Fade(FX));
		const double B = FMath::Lerp(Gradient(Hash(IX, IY + 1, Seed), FX, FY - 1.0), Gradient(Hash(IX + 1, IY + 1, Seed), FX - 1.0, FY - 1.0), Fade(FX));
		return FMath::Lerp(A, B, Fade(FY));
	}

}
