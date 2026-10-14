#include "Terrain/ChopItTerrainAssetCustomization.h"
#include "Terrain/ChopItTerrainProjectSettings.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

void FChopItTerrainAssetCustomization::CustomizeDetails(IDetailLayoutBuilder& Builder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	Builder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1) return;
	const TWeakObjectPtr<UChopItTerrainDataAsset> Asset = Cast<UChopItTerrainDataAsset>(Objects[0].Get());
	if (!Asset.IsValid()) return;
	Builder.EditCategory(TEXT("Project Default"), FText::GetEmpty(), ECategoryPriority::Important)
		.AddCustomRow(NSLOCTEXT("ChopItTerrain", "DefaultSearch", "Set as Default Terrain"))
		.WholeRowContent()
		[
			SNew(SButton)
			.Text_Lambda([Asset]()
			{
				return Asset.IsValid() && GetDefault<UChopItTerrainProjectSettings>()->DefaultTerrain.ToSoftObjectPath() == FSoftObjectPath(Asset.Get())
					? NSLOCTEXT("ChopItTerrain", "CurrentDefault", "Current Default Terrain")
					: NSLOCTEXT("ChopItTerrain", "SetDefault", "Set as Default Terrain");
			})
			.ToolTipText(NSLOCTEXT("ChopItTerrain", "SetDefaultTip", "Use this recipe for worlds that generate the project default terrain. Each run supplies its own seed. Save asset edits before playing."))
			.OnClicked_Lambda([Asset]()
			{
				if (!Asset.IsValid()) return FReply::Handled();
				UChopItTerrainProjectSettings* Settings = GetMutableDefault<UChopItTerrainProjectSettings>();
				const auto Previous = Settings->DefaultTerrain;
				Settings->DefaultTerrain = Asset.Get();
				const bool bSaved = Settings->TryUpdateDefaultConfigFile();
				if (!bSaved) Settings->DefaultTerrain = Previous;
				FNotificationInfo Info(bSaved
					? NSLOCTEXT("ChopItTerrain", "DefaultSaved", "Default terrain saved for this project.")
					: NSLOCTEXT("ChopItTerrain", "DefaultFailed", "Could not save Config/DefaultGame.ini. Check that the file is writable."));
				Info.ExpireDuration = 4.0f;
				FSlateNotificationManager::Get().AddNotification(Info);
				return FReply::Handled();
			})
		];
}
