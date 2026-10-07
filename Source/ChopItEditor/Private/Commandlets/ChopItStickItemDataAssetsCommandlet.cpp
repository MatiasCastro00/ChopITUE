#include "Commandlets/ChopItStickItemDataAssetsCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Items/ChopItItemDataAsset.h"
#include "Engine/Texture2D.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

int32 UChopItStickItemDataAssetsCommandlet::Main(const FString& Params)
{
	struct FStickAsset { const TCHAR* Id; const TCHAR* DisplayName; const TCHAR* IconId; };
	const FStickAsset Sticks[] = {
		{TEXT("CharredStick"), TEXT("Charred Stick"), TEXT("CharredStick")},
		{TEXT("DebtStick"), TEXT("Debt Stick"), TEXT("DebtStick")},
		{TEXT("EchoStick"), TEXT("Echo Stick"), TEXT("EchoStick")},
		{TEXT("ForestHeart"), TEXT("Forest Heart"), TEXT("ForestHeart")},
		{TEXT("GnarledStick"), TEXT("Gnarled Stick"), TEXT("GnarledStick")},
		{TEXT("LightningRodStick"), TEXT("Lightning Rod"), TEXT("LightningRodStick")},
		{TEXT("LightStick"), TEXT("Light Stick"), TEXT("LightStick")},
		{TEXT("LuckyStick"), TEXT("Lucky Stick"), TEXT("LuckyStick")},
		{TEXT("MagnetizedStick"), TEXT("Magnetized Stick"), TEXT("MagnetizedStick")},
		{TEXT("MandrakeStick"), TEXT("Mandrake Stick"), TEXT("MandrakeStick")},
		{TEXT("MidnightStick"), TEXT("Midnight Stick"), TEXT("MidnightStick")},
		{TEXT("MushroomStick"), TEXT("Mushroom Stick"), TEXT("MushroomStick")},
		{TEXT("ObserverStick"), TEXT("Observer Stick"), TEXT("ObserverStick")},
		{TEXT("ReaperStick"), TEXT("Reaper Stick"), TEXT("ReaperStick")},
		{TEXT("ReturnStick"), TEXT("Return Stick"), TEXT("ReturnStick")},
		{TEXT("RootedStick"), TEXT("Rooted Stick"), TEXT("RootedStick")},
		{TEXT("SplinteredStick"), TEXT("Splintered Stick"), TEXT("SplinteredStick")},
		{TEXT("StickyStick"), TEXT("Sticky Stick"), TEXT("StickyStick")},
		{TEXT("ThickBarkStick"), TEXT("Thick Bark Stick"), TEXT("ThickBarkStick")},
		{TEXT("YggdrasilStick"), TEXT("Yggdrasil Stick"), TEXT("YggdrasilStick")},
	};

	for (const FStickAsset& Stick : Sticks)
	{
		const FString AssetName = FString(TEXT("DA_Item_")) + Stick.Id;
		const FString PackagePath = TEXT("/Game/ChopIt/Items/") + AssetName;
		if (FPackageName::DoesPackageExist(PackagePath)) continue;

		UPackage* Package = CreatePackage(*PackagePath);
		if (!Package) return 1;
		UChopItItemDataAsset* Item = NewObject<UChopItItemDataAsset>(Package, *AssetName, RF_Public | RF_Standalone);
		Item->ItemId = FName(Stick.Id);
		Item->DisplayName = FText::FromString(Stick.DisplayName);
		Item->Description = FText::FromString(TEXT("Placeholder: configure this passive effect and description before enabling loot."));
		Item->SpawnWeight = 0.f; // Keep unfinished examples out of reward drops.
		const FString IconName = FString(TEXT("T_Stick_")) + Stick.IconId;
		const FString IconPath = FString::Printf(TEXT("/Game/ChopIt/Art/Sticks/%s.%s"), *IconName, *IconName);
		Item->Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconPath));

		FAssetRegistryModule::AssetCreated(Item);
		Package->MarkPackageDirty();
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
		if (!UPackage::SavePackage(Package, Item, *Filename, SaveArgs))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save stick Data Asset: %s"), *PackagePath);
			return 1;
		}
		UE_LOG(LogTemp, Display, TEXT("Created %s with icon %s (SpawnWeight=0)."), *PackagePath, *IconPath);
	}
	return 0;
}
