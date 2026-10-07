#include "Commandlets/ChopItItemExamplesCommandlet.h"
#include "Items/ChopItItemDataAsset.h"
#include "Items/ChopItPassiveEffects.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

int32 UChopItItemExamplesCommandlet::Main(const FString& Params)
{
	struct FExample { const TCHAR* Id; const TCHAR* Name; const TCHAR* Description; UClass* Effect; EChopItItemRarity Rarity; };
	const FExample Examples[] = {
		{TEXT("Heartwood"), TEXT("Palo Corazón"), TEXT("Regenera 1 punto de vida por segundo. Cada stack adicional aporta la mitad."), UChopItRegenerationEffect::StaticClass(), EChopItItemRarity::Common},
		{TEXT("VampireStick"), TEXT("Palo Vampiro"), TEXT("Cura un 10% del daño realizado, excluyendo ejecuciones. Cada stack adicional aporta la mitad."), UChopItLifeStealEffect::StaticClass(), EChopItItemRarity::Uncommon},
		{TEXT("GenerousLog"), TEXT("Tronco Generoso"), TEXT("Recibe un 10% adicional de madera, sujeto a capacidad. Cada stack adicional aporta la mitad."), UChopItWoodYieldEffect::StaticClass(), EChopItItemRarity::Common},
		{TEXT("Wormwood"), TEXT("Palo Carcomido"), TEXT("El 10% del daño genera infestación. Ejecuta al objetivo cuando su vida no supera la infestación. Cada stack adicional aporta la mitad."), UChopItInfestationEffect::StaticClass(), EChopItItemRarity::Rare}
	};
	for (const auto& Example : Examples)
	{
		const FString Name = FString(TEXT("DA_Item_")) + Example.Id;
		const FString Path = TEXT("/Game/ChopIt/Items/") + Name;
		if (FPackageName::DoesPackageExist(Path)) continue;
		UPackage* Package = CreatePackage(*Path);
		auto* Item = NewObject<UChopItItemDataAsset>(Package, *Name, RF_Public | RF_Standalone);
		Item->ItemId = Example.Id; Item->DisplayName = FText::FromString(Example.Name);
		Item->Description = FText::FromString(Example.Description); Item->Rarity = Example.Rarity;
		Item->Effects.Add(NewObject<UChopItItemEffect>(Item, Example.Effect, NAME_None, RF_Transactional));
		FAssetRegistryModule::AssetCreated(Item);
		Package->MarkPackageDirty();
		FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
		if (!UPackage::SavePackage(Package, Item, *FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension()), Args)) return 1;
	}
	return 0;
}
