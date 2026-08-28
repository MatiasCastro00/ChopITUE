#include "Weapons/ChopItWeaponDefinition.h"

FPrimaryAssetId UChopItWeaponDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ChopItWeapon"), GetFName());
}
