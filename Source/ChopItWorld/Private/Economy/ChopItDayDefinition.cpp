#include "Economy/ChopItDayDefinition.h"

FPrimaryAssetId UChopItDayDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ChopItDay"), GetFName());
}
