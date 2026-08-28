#include "ChopItAssetManager.h"
#include "ChopItCollision.h"
#include "ChopItDeveloperSettings.h"
#include "ChopItGameplayTags.h"
#include "Engine/CollisionProfile.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItProjectConfigurationSmokeTest,
	"ChopIt.Smoke.ProjectConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItProjectConfigurationSmokeTest::RunTest(const FString& Parameters)
{
	UAssetManager& AssetManager = UAssetManager::Get();
	TestTrue(TEXT("Custom Asset Manager is active"), AssetManager.IsA<UChopItAssetManager>());

	const UChopItDeveloperSettings* Settings = GetDefault<UChopItDeveloperSettings>();
	TestNotNull(TEXT("Developer settings are available"), Settings);
	if (Settings)
	{
		TestEqual(
			TEXT("Startup map reference is configured"),
			Settings->StartupMap.ToString(),
			FString(TEXT("/Game/ChopIt/World/Maps/L_Startup.L_Startup")));
	}

	const TArray<FString> RequiredMapPackages =
	{
		TEXT("/Game/ChopIt/World/Maps/L_Startup"),
		TEXT("/Game/ChopIt/World/Maps/L_Dev_Sandbox")
	};

	for (const FString& MapPackage : RequiredMapPackages)
	{
		TestTrue(
			FString::Printf(TEXT("Map package exists: %s"), *MapPackage),
			FPackageName::DoesPackageExist(MapPackage));
	}

	TArray<FPrimaryAssetId> DiscoveredMaps;
	TestTrue(
		TEXT("Asset Manager discovers configured maps"),
		AssetManager.GetPrimaryAssetIdList(UAssetManager::MapType, DiscoveredMaps));
	TestTrue(
		TEXT("Asset Manager discovers L_Startup"),
		DiscoveredMaps.Contains(FPrimaryAssetId(UAssetManager::MapType, TEXT("/Game/ChopIt/World/Maps/L_Startup"))));
	TestTrue(
		TEXT("Asset Manager discovers L_Dev_Sandbox"),
		DiscoveredMaps.Contains(FPrimaryAssetId(UAssetManager::MapType, TEXT("/Game/ChopIt/World/Maps/L_Dev_Sandbox"))));

	TestTrue(TEXT("Day phase tag is registered"), ChopItGameplayTags::State_Cycle_Day.GetTag().IsValid());
	TestTrue(TEXT("Night phase tag is registered"), ChopItGameplayTags::State_Cycle_Night.GetTag().IsValid());
	TestTrue(TEXT("Death phase tag is registered"), ChopItGameplayTags::State_Run_Death.GetTag().IsValid());

	const TArray<FName> RequiredModules =
	{
		TEXT("ChopItCore"),
		TEXT("ChopItCombat"),
		TEXT("ChopItWorld"),
		TEXT("ChopItAI"),
		TEXT("ChopItMeta"),
		TEXT("ChopIt"),
		TEXT("ChopItPresentation")
	};

	for (const FName ModuleName : RequiredModules)
	{
		TestTrue(
			FString::Printf(TEXT("Required module is loaded: %s"), *ModuleName.ToString()),
			FModuleManager::Get().IsModuleLoaded(ModuleName));
	}

	const UCollisionProfile* CollisionProfile = UCollisionProfile::Get();
	TestNotNull(TEXT("Collision profile service is available"), CollisionProfile);
	if (CollisionProfile)
	{
		const TArray<FName> RequiredProfiles =
		{
			ChopItCollisionProfiles::Player,
			ChopItCollisionProfiles::Enemy,
			ChopItCollisionProfiles::Harvestable,
			ChopItCollisionProfiles::Projectile,
			ChopItCollisionProfiles::Pickup,
			ChopItCollisionProfiles::DeliveryZone
		};

		for (const FName ProfileName : RequiredProfiles)
		{
			FCollisionResponseTemplate ProfileTemplate;
			TestTrue(
				FString::Printf(TEXT("Collision profile is registered: %s"), *ProfileName.ToString()),
				CollisionProfile->GetProfileTemplate(ProfileName, ProfileTemplate));
		}

		TestEqual(TEXT("Enemy channel name"), CollisionProfile->ReturnChannelNameFromContainerIndex(ChopItCollisionChannels::Enemy), FName(TEXT("ChopItEnemy")));
		TestEqual(TEXT("Harvestable channel name"), CollisionProfile->ReturnChannelNameFromContainerIndex(ChopItCollisionChannels::Harvestable), FName(TEXT("ChopItHarvestable")));
		TestEqual(TEXT("Projectile channel name"), CollisionProfile->ReturnChannelNameFromContainerIndex(ChopItCollisionChannels::Projectile), FName(TEXT("ChopItProjectile")));
		TestEqual(TEXT("Pickup channel name"), CollisionProfile->ReturnChannelNameFromContainerIndex(ChopItCollisionChannels::Pickup), FName(TEXT("ChopItPickup")));
		TestEqual(TEXT("Delivery zone channel name"), CollisionProfile->ReturnChannelNameFromContainerIndex(ChopItCollisionChannels::DeliveryZone), FName(TEXT("ChopItDeliveryZone")));
		TestEqual(TEXT("Interaction channel name"), CollisionProfile->ReturnChannelNameFromContainerIndex(ChopItCollisionChannels::Interaction), FName(TEXT("ChopItInteraction")));
	}

	return true;
}

#endif
