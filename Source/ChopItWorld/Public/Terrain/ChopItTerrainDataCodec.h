#pragma once

#include "CoreMinimal.h"
#include "Terrain/ChopItHeightField.h"

/** Versioned binary snapshot of the complete height field and analysis maps. */
class CHOPITWORLD_API FChopItTerrainDataCodec
{
public:
	static bool Encode(const FChopItHeightField& Field, TArray<uint8>& OutData, FString& OutError);
	static bool Decode(const TArray<uint8>& Data, FChopItHeightField& OutField, FString& OutError);
private:
	static void Transfer(FArchive& Ar, FChopItHeightField& Field);
	static void Write(FArchive& Ar, const FChopItHeightField& Field);
	static bool Validate(const FChopItHeightField& Field);
};
