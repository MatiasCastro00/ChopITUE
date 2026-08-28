#include "ChopItAssetManager.h"

#include "ChopItLogChannels.h"

UChopItAssetManager& UChopItAssetManager::Get()
{
	UAssetManager& AssetManager = UAssetManager::Get();
	UChopItAssetManager* ChopItAssetManager = Cast<UChopItAssetManager>(&AssetManager);
	checkf(ChopItAssetManager, TEXT("AssetManagerClassName must be configured as UChopItAssetManager."));
	return *ChopItAssetManager;
}

void UChopItAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	UE_LOG(LogChopIt, Log, TEXT("ChopIt Asset Manager initialized."));
}
