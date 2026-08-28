#include "Progression/ChopItUpgradeDefinition.h"

FPrimaryAssetId UChopItUpgradeDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ChopItUpgrade"), UpgradeId.IsNone() ? GetFName() : UpgradeId);
}
