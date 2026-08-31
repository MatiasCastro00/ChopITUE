#include "Commandlets/ChopItBootstrapCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "ChopItLogChannels.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/Blueprint.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "Economy/ChopItCabinHub.h"
#include "Economy/ChopItChainDefinition.h"
#include "Economy/ChopItDayDefinition.h"
#include "Economy/ChopItDeliveryZone.h"
#include "Economy/ChopItQuotaMachine.h"
#include "Economy/ChopItSellZone.h"
#include "Enemies/ChopItEnemyCharacter.h"
#include "Enemies/ChopItEnemyDefinition.h"
#include "GameFramework/WorldSettings.h"
#include "Harvest/ChopItTree.h"
#include "Harvest/ChopItLogPickup.h"
#include "EngineUtils.h"
#include "EnhancedActionKeyMapping.h"
#include "FileHelpers.h"
#include "Framework/ChopItGameMode.h"
#include "GameFramework/PlayerStart.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Player/ChopItCharacter.h"
#include "Progression/ChopItUpgradeDefinition.h"
#include "Shop/ChopItShopTerminal.h"
#include "Spawning/ChopItEnemyDirectorDefinition.h"
#include "Pacts/ChopItPactDefinition.h"
#include "Targets/ChopItCombatDummy.h"
#include "UObject/SavePackage.h"
#include "Weapons/ChopItWeaponDefinition.h"

namespace ChopItBootstrap
{
	constexpr TCHAR StartupMap[] = TEXT("/Game/ChopIt/World/Maps/L_Startup");
	constexpr TCHAR SandboxMap[] = TEXT("/Game/ChopIt/World/Maps/L_Dev_Sandbox");
	constexpr TCHAR CombatMap[] = TEXT("/Game/ChopIt/World/Maps/L_Test_Combat");
	constexpr TCHAR HarvestMap[] = TEXT("/Game/ChopIt/World/Maps/L_Test_Harvest");
	constexpr TCHAR EconomyMap[] = TEXT("/Game/ChopIt/World/Maps/L_Test_Economy");
	constexpr TCHAR CycleMap[] = TEXT("/Game/ChopIt/World/Maps/L_Test_Cycle");
	constexpr TCHAR ProgressionMap[] = TEXT("/Game/ChopIt/World/Maps/L_Test_Progression");
	constexpr TCHAR ShopMap[] = TEXT("/Game/ChopIt/World/Maps/L_Test_Shop");
	constexpr TCHAR EnemyMap[] = TEXT("/Game/ChopIt/World/Maps/L_Test_Enemies");
	constexpr TCHAR ChainLabMap[] = TEXT("/Game/ChopIt/World/Maps/L_Test_ChainLab");
	constexpr TCHAR MoveActionPackage[] = TEXT("/Game/ChopIt/Input/IA_Move");
	constexpr TCHAR InteractActionPackage[] = TEXT("/Game/ChopIt/Input/IA_Interact");
	constexpr TCHAR GameplayContextPackage[] = TEXT("/Game/ChopIt/Input/IMC_Gameplay");
	constexpr TCHAR CharacterBlueprintPackage[] = TEXT("/Game/ChopIt/Characters/Blueprints/BP_ChopItCharacter");
	constexpr TCHAR BasicAxePackage[] = TEXT("/Game/ChopIt/Combat/Weapons/DA_Weapon_BasicAxe");
	constexpr TCHAR HandSawPackage[] = TEXT("/Game/ChopIt/Combat/Weapons/DA_Weapon_HandSaw");
	constexpr TCHAR SawHaloPackage[] = TEXT("/Game/ChopIt/Combat/Weapons/DA_Weapon_SawHalo");
	constexpr TCHAR ShopBlueprintPackage[] = TEXT("/Game/ChopIt/World/Economy/BP_ShopTerminal");
	constexpr TCHAR TreeBlueprintPackage[] = TEXT("/Game/ChopIt/World/Forest/Trees/BP_Tree_Basic");
	constexpr TCHAR LogPickupBlueprintPackage[] = TEXT("/Game/ChopIt/World/Pickups/BP_LogPickup");
	constexpr TCHAR CabinBlueprintPackage[] = TEXT("/Game/ChopIt/World/Economy/BP_CabinHub");
	constexpr TCHAR QuotaMachineBlueprintPackage[] = TEXT("/Game/ChopIt/World/Economy/BP_QuotaMachine");
	constexpr TCHAR DeliveryZoneBlueprintPackage[] = TEXT("/Game/ChopIt/World/Economy/BP_DeliveryZone");
	constexpr TCHAR SellZoneBlueprintPackage[] = TEXT("/Game/ChopIt/World/Economy/BP_SellZone");
	constexpr TCHAR ChainLabMachineBlueprintPackage[] = TEXT("/Game/ChopIt/World/ChainLab/BP_ChainLabMachine");
	constexpr TCHAR DefaultChainDefinitionPackage[] = TEXT("/Game/ChopIt/World/ChainLab/DA_Chain_Default");
	constexpr TCHAR DayOnePackage[] = TEXT("/Game/ChopIt/Economy/Days/DA_Day_01");

	template <typename AssetType>
	AssetType* LoadOrCreateAsset(const FString& PackageName, const FName AssetName)
	{
		UPackage* Package = nullptr;
		if (FPackageName::DoesPackageExist(PackageName))
		{
			Package = LoadPackage(nullptr, *PackageName, LOAD_None);
		}
		if (!Package)
		{
			Package = CreatePackage(*PackageName);
		}

		AssetType* Asset = FindObject<AssetType>(Package, *AssetName.ToString());
		if (!Asset)
		{
			Asset = NewObject<AssetType>(Package, AssetName, RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(Asset);
		}
		return Asset;
	}

	bool SaveAsset(UObject* Asset)
	{
		if (!Asset)
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(),
			FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.Error = GError;
		return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
	}

	UMaterialInterface* LoadBlockoutMaterial(const TCHAR* Name)
	{
		return LoadObject<UMaterialInterface>(
			nullptr,
			*FString::Printf(TEXT("/Game/ChopIt/World/Blockout/Materials/%s.%s"), Name, Name));
	}

	AStaticMeshActor* SpawnBlockoutMesh(
		UWorld* World,
		UStaticMesh* Mesh,
		const FName Name,
		const FVector& Location,
		const FVector& Scale,
		UMaterialInterface* Material,
		const FName CollisionProfile = TEXT("BlockAll"),
		const FRotator& Rotation = FRotator::ZeroRotator)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = Name;
		AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Location, Rotation, SpawnParameters);
		if (!Actor)
		{
			return nullptr;
		}

		Actor->SetActorLabel(Name.ToString());
		Actor->SetActorScale3D(Scale);
		UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
		Component->SetStaticMesh(Mesh);
		Component->SetMaterial(0, Material);
		Component->SetMobility(EComponentMobility::Static);
		Component->SetCollisionProfileName(CollisionProfile);
		return Actor;
	}

	ATextRenderActor* SpawnLabSign(
		UWorld* World,
		const FName Name,
		const FString& Text,
		const FVector& Location,
		const FRotator& Rotation = FRotator(0.0f, 180.0f, 0.0f))
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = Name;
		ATextRenderActor* Actor = World->SpawnActor<ATextRenderActor>(Location, Rotation, SpawnParameters);
		if (!Actor)
		{
			return nullptr;
		}

		Actor->SetActorLabel(Name.ToString());
		UTextRenderComponent* TextComponent = Actor->GetTextRender();
		TextComponent->SetText(FText::FromString(Text));
		TextComponent->SetHorizontalAlignment(EHTA_Center);
		TextComponent->SetWorldSize(48.0f);
		TextComponent->SetTextRenderColor(FColor(255, 220, 110));
		TextComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Actor;
	}
}

UChopItBootstrapCommandlet::UChopItBootstrapCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 UChopItBootstrapCommandlet::Main(const FString& Params)
{
	const TArray<FString> RequiredMaps =
	{
		ChopItBootstrap::StartupMap,
		ChopItBootstrap::SandboxMap
	};

	for (const FString& MapPackage : RequiredMaps)
	{
		if (!CreateMapIfMissing(MapPackage))
		{
			UE_LOG(LogChopIt, Error, TEXT("Failed to create required map %s."), *MapPackage);
			return 1;
		}
	}

	if (FParse::Param(*Params, TEXT("Phase1")) && !CreatePhase1Assets())
	{
		UE_LOG(LogChopIt, Error, TEXT("Failed to create Phase 1 assets."));
		return 1;
	}

	if (FParse::Param(*Params, TEXT("Phase2")) && !CreatePhase2Assets())
	{
		UE_LOG(LogChopIt, Error, TEXT("Failed to create Phase 2 assets."));
		return 1;
	}

	if (FParse::Param(*Params, TEXT("Phase3")) && !CreatePhase3Assets())
	{
		UE_LOG(LogChopIt, Error, TEXT("Failed to create Phase 3 assets."));
		return 1;
	}

	if (FParse::Param(*Params, TEXT("Phase4")) && !CreatePhase4Assets())
	{
		UE_LOG(LogChopIt, Error, TEXT("Failed to create Phase 4 assets."));
		return 1;
	}

	if (FParse::Param(*Params, TEXT("Phase5")) && !CreatePhase5Assets())
	{
		UE_LOG(LogChopIt, Error, TEXT("Failed to create Phase 5 assets."));
		return 1;
	}

	if (FParse::Param(*Params, TEXT("Phase6")) && !CreatePhase6Assets())
	{
		UE_LOG(LogChopIt, Error, TEXT("Failed to create Phase 6 assets."));
		return 1;
	}

	if (FParse::Param(*Params, TEXT("Phase7")) && !CreatePhase7Assets())
	{
		UE_LOG(LogChopIt, Error, TEXT("Failed to create Phase 7 assets."));
		return 1;
	}

	if (FParse::Param(*Params, TEXT("Phase8")) && !CreatePhase8Assets())
	{
		UE_LOG(LogChopIt, Error, TEXT("Failed to create Phase 8 assets."));
		return 1;
	}
	if (FParse::Param(*Params, TEXT("Phase9")) && !CreatePhase9Assets())
	{
		UE_LOG(LogChopIt, Error, TEXT("Failed to create Phase 9 assets."));
		return 1;
	}
	if (FParse::Param(*Params, TEXT("Phase10")) && !CreatePhase10Assets()) return 1;
	if (FParse::Param(*Params, TEXT("Phase12")) && !CreatePhase12Assets()) return 1;
	if (FParse::Param(*Params, TEXT("ChainLab")) && !CreateChainLabAssets())
	{
		UE_LOG(LogChopIt, Error, TEXT("Failed to create Chain Lab assets."));
		return 1;
	}

	UE_LOG(LogChopIt, Display, TEXT("ChopIt bootstrap assets are ready."));
	return 0;
}

bool UChopItBootstrapCommandlet::CreateMapIfMissing(const FString& LongPackageName) const
{
	if (!FPackageName::IsValidLongPackageName(LongPackageName))
	{
		UE_LOG(LogChopIt, Error, TEXT("Invalid map package name: %s"), *LongPackageName);
		return false;
	}

	if (FPackageName::DoesPackageExist(LongPackageName))
	{
		UE_LOG(LogChopIt, Display, TEXT("Required map already exists: %s"), *LongPackageName);
		return true;
	}

	UWorld* NewWorld = UEditorLoadingAndSavingUtils::NewBlankMap(false);
	if (!IsValid(NewWorld))
	{
		UE_LOG(LogChopIt, Error, TEXT("Could not allocate a blank world for %s."), *LongPackageName);
		return false;
	}

	const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(NewWorld, LongPackageName);
	UE_LOG(LogChopIt, Display, TEXT("Created non-partitioned map %s: %s"), *LongPackageName, bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}

bool UChopItBootstrapCommandlet::CreatePhase1Assets() const
{
	return CreateBlockoutMaterials()
		&& CreateInputAssets()
		&& CreateCharacterBlueprint()
		&& RebuildPhase1Map(ChopItBootstrap::StartupMap, false)
		&& RebuildPhase1Map(ChopItBootstrap::SandboxMap, true);
}

bool UChopItBootstrapCommandlet::CreatePhase2Assets() const
{
	return CreateBasicAxeAsset()
		&& RebuildPhase1Map(ChopItBootstrap::CombatMap, true, true);
}

bool UChopItBootstrapCommandlet::CreateBasicAxeAsset() const
{
	UChopItWeaponDefinition* BasicAxe = ChopItBootstrap::LoadOrCreateAsset<UChopItWeaponDefinition>(
		ChopItBootstrap::BasicAxePackage,
		TEXT("DA_Weapon_BasicAxe"));
	if (!BasicAxe)
	{
		return false;
	}

	BasicAxe->DisplayName = FText::FromString(TEXT("Hacha básica"));
	BasicAxe->Description = FText::FromString(TEXT("Arma exclusiva del leñador"));
	BasicAxe->WeaponId = TEXT("BasicAxe");
	BasicAxe->AttackPattern = EChopItWeaponAttackPattern::ArcMelee;
	BasicAxe->bExclusiveToStartingCharacter = true;
	BasicAxe->bUsesLoadoutSlot = false;
	BasicAxe->ShopPrice = 0;
	BasicAxe->Damage = 25.0f;
	BasicAxe->AttackInterval = 0.65f;
	BasicAxe->Range = 500.0f;
	BasicAxe->ArcHalfAngleDegrees = 55.0f;
	BasicAxe->MaxTargets = 3;
	BasicAxe->CriticalChance = 0.0f;
	BasicAxe->CriticalMultiplier = 2.0f;
	const bool bSaved = ChopItBootstrap::SaveAsset(BasicAxe);
	UE_LOG(LogChopIt, Display, TEXT("Phase 2 basic axe asset: %s"), bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}

bool UChopItBootstrapCommandlet::CreateSharedWeaponAssets() const
{
	struct FWeaponSpec
	{
		const TCHAR* Package;
		const TCHAR* AssetName;
		const TCHAR* WeaponId;
		const TCHAR* DisplayName;
		const TCHAR* Description;
		EChopItWeaponAttackPattern Pattern;
		int64 Price;
		float Damage;
		float Interval;
		float Range;
		float Arc;
		int32 Targets;
	};
	const FWeaponSpec Specs[] =
	{
		{ ChopItBootstrap::HandSawPackage, TEXT("DA_Weapon_HandSaw"), TEXT("HandSaw"), TEXT("Motosierra de mano"), TEXT("Corta rapido en un arco corto"), EChopItWeaponAttackPattern::ArcMelee, 20, 13.0f, 0.22f, 320.0f, 70.0f, 2 },
		{ ChopItBootstrap::SawHaloPackage, TEXT("DA_Weapon_SawHalo"), TEXT("SawHalo"), TEXT("Sierra circular"), TEXT("Golpea en todas direcciones"), EChopItWeaponAttackPattern::RadialMelee, 32, 18.0f, 0.55f, 390.0f, 180.0f, 5 }
	};
	for (const FWeaponSpec& Spec : Specs)
	{
		UChopItWeaponDefinition* Weapon = ChopItBootstrap::LoadOrCreateAsset<UChopItWeaponDefinition>(Spec.Package, Spec.AssetName);
		if (!Weapon) { return false; }
		Weapon->WeaponId = Spec.WeaponId;
		Weapon->DisplayName = FText::FromString(Spec.DisplayName);
		Weapon->Description = FText::FromString(Spec.Description);
		Weapon->AttackPattern = Spec.Pattern;
		Weapon->bExclusiveToStartingCharacter = false;
		Weapon->bUsesLoadoutSlot = true;
		Weapon->ShopPrice = Spec.Price;
		Weapon->Damage = Spec.Damage;
		Weapon->AttackInterval = Spec.Interval;
		Weapon->Range = Spec.Range;
		Weapon->ArcHalfAngleDegrees = Spec.Arc;
		Weapon->MaxTargets = Spec.Targets;
		Weapon->CriticalChance = 0.0f;
		Weapon->CriticalMultiplier = 2.0f;
		if (!ChopItBootstrap::SaveAsset(Weapon)) { return false; }
	}
	UE_LOG(LogChopIt, Display, TEXT("Phase 7 shared weapons: OK"));
	return true;
}

bool UChopItBootstrapCommandlet::CreateShopBlueprint() const
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ChopIt/World/Economy/BP_ShopTerminal.BP_ShopTerminal"));
	if (!Blueprint)
	{
		UPackage* Package = CreatePackage(ChopItBootstrap::ShopBlueprintPackage);
		Blueprint = FKismetEditorUtilities::CreateBlueprint(AChopItShopTerminal::StaticClass(), Package, TEXT("BP_ShopTerminal"), BPTYPE_Normal, NAME_None);
		if (Blueprint) { FAssetRegistryModule::AssetCreated(Blueprint); }
	}
	if (!Blueprint) { return false; }
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	return ChopItBootstrap::SaveAsset(Blueprint);
}

bool UChopItBootstrapCommandlet::CreatePhase3Assets() const
{
	return CreateHarvestBlueprints()
		&& RebuildPhase1Map(ChopItBootstrap::HarvestMap, true, false, true);
}

bool UChopItBootstrapCommandlet::CreateHarvestBlueprints() const
{
	struct FBlueprintSpec
	{
		const TCHAR* ObjectPath;
		const TCHAR* PackagePath;
		const TCHAR* AssetName;
		UClass* ParentClass;
	};
	const FBlueprintSpec Specs[] =
	{
		{
			TEXT("/Game/ChopIt/World/Forest/Trees/BP_Tree_Basic.BP_Tree_Basic"),
			ChopItBootstrap::TreeBlueprintPackage,
			TEXT("BP_Tree_Basic"),
			AChopItTree::StaticClass()
		},
		{
			TEXT("/Game/ChopIt/World/Pickups/BP_LogPickup.BP_LogPickup"),
			ChopItBootstrap::LogPickupBlueprintPackage,
			TEXT("BP_LogPickup"),
			AChopItLogPickup::StaticClass()
		}
	};

	for (const FBlueprintSpec& Spec : Specs)
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, Spec.ObjectPath);
		if (!Blueprint)
		{
			UPackage* Package = CreatePackage(Spec.PackagePath);
			Blueprint = FKismetEditorUtilities::CreateBlueprint(
				Spec.ParentClass,
				Package,
				FName(Spec.AssetName),
				BPTYPE_Normal,
				NAME_None);
			if (Blueprint)
			{
				FAssetRegistryModule::AssetCreated(Blueprint);
			}
		}
		if (!Blueprint)
		{
			return false;
		}
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (!ChopItBootstrap::SaveAsset(Blueprint))
		{
			return false;
		}
	}
	UE_LOG(LogChopIt, Display, TEXT("Phase 3 harvest Blueprints: OK"));
	return true;
}

bool UChopItBootstrapCommandlet::CreatePhase4Assets() const
{
	return CreateDayDefinition()
		&& CreateEconomyBlueprints()
		&& RebuildPhase1Map(ChopItBootstrap::EconomyMap, true, false, false, true);
}

bool UChopItBootstrapCommandlet::CreatePhase5Assets() const
{
	return CreateDayDefinition()
		&& CreateEconomyBlueprints()
		&& RebuildPhase1Map(ChopItBootstrap::CycleMap, true, false, false, true);
}

bool UChopItBootstrapCommandlet::CreatePhase6Assets() const
{
	return CreateDayDefinition()
		&& CreateEconomyBlueprints()
		&& CreateProgressionAssets()
		&& RebuildPhase1Map(ChopItBootstrap::ProgressionMap, true, false, false, true);
}

bool UChopItBootstrapCommandlet::CreatePhase7Assets() const
{
	return CreateBasicAxeAsset()
		&& CreateSharedWeaponAssets()
		&& CreateShopBlueprint()
		&& RebuildPhase1Map(ChopItBootstrap::ShopMap, true, false, false, true);
}

bool UChopItBootstrapCommandlet::CreatePhase8Assets() const
{
	return CreateEnemyAssets()
		&& RebuildPhase1Map(ChopItBootstrap::EnemyMap, true, false, false, true);
}

bool UChopItBootstrapCommandlet::CreatePhase9Assets() const
{
	auto Configure = [](const TCHAR* Package, const TCHAR* Asset, const TCHAR* Id, const TCHAR* Name, float Health, float Speed, float Damage, int32 XP)
	{
		UChopItEnemyDefinition* Enemy = ChopItBootstrap::LoadOrCreateAsset<UChopItEnemyDefinition>(Package, Asset);
		if (!Enemy) { return false; }
		Enemy->EnemyId = FName(Id);
		Enemy->DisplayName = FText::FromString(Name);
		Enemy->MaxHealth = Health;
		Enemy->MoveSpeed = Speed;
		Enemy->ContactDamage = Damage;
		Enemy->AttackInterval = 0.7f;
		Enemy->AttackRange = 130.0f;
		Enemy->ExperienceReward = XP;
		Enemy->WoodRewardUnits = 3;
		return ChopItBootstrap::SaveAsset(Enemy);
	};
	const bool bGuard = Configure(TEXT("/Game/ChopIt/AI/Enemies/DA_Enemy_Guardian"), TEXT("DA_Enemy_Guardian"), TEXT("Guardian"), TEXT("Guardian del bosque"), 260.0f, 210.0f, 18.0f, 30);
	const bool bFinal = Configure(TEXT("/Game/ChopIt/AI/Enemies/DA_Enemy_ForestEntity"), TEXT("DA_Enemy_ForestEntity"), TEXT("ForestEntity"), TEXT("Entidad del bosque"), 520.0f, 260.0f, 24.0f, 75);
	return bGuard && bFinal && RebuildPhase1Map(TEXT("/Game/ChopIt/World/Maps/L_Test_Elite"), true, false, false, true);
}

bool UChopItBootstrapCommandlet::CreatePhase10Assets() const
{
	auto Make=[](const TCHAR* Id,const TCHAR* Name,const TCHAR* Desc,const TArray<FChopItStatModifier>& Modifiers)
	{ FString Asset=FString::Printf(TEXT("DA_Pact_%s"),Id); UChopItPactDefinition* P=ChopItBootstrap::LoadOrCreateAsset<UChopItPactDefinition>(FString::Printf(TEXT("/Game/ChopIt/Pacts/%s"),*Asset),FName(*Asset)); if(!P)return false; P->PactId=FName(Id);P->DisplayName=FText::FromString(Name);P->Description=FText::FromString(Desc);P->CurseIncrease=1;P->Modifiers=Modifiers;return ChopItBootstrap::SaveAsset(P); };
	auto Mod=[](EChopItCombatStat S,float V){ FChopItStatModifier M; M.Stat=S;M.Operation=EChopItModifierOperation::Multiply;M.Magnitude=V;return M; };
	return Make(TEXT("Furia"),TEXT("Furia de la Parca"),TEXT("+35% dano  |  -15% cadencia"),{Mod(EChopItCombatStat::Damage,1.35f),Mod(EChopItCombatStat::AttackSpeed,0.85f)})
		&& Make(TEXT("Ritmo"),TEXT("Ritmo funebre"),TEXT("+25% cadencia  |  -10% velocidad"),{Mod(EChopItCombatStat::AttackSpeed,1.25f),Mod(EChopItCombatStat::MovementSpeed,0.90f)})
		&& Make(TEXT("Botas"),TEXT("Paso espectral"),TEXT("+20% velocidad  |  -15% alcance"),{Mod(EChopItCombatStat::MovementSpeed,1.20f),Mod(EChopItCombatStat::Range,0.85f)});
}

bool UChopItBootstrapCommandlet::CreatePhase12Assets() const
{
	return CreatePhase9Assets()
		&& RebuildPhase1Map(TEXT("/Game/ChopIt/World/Maps/L_Test_Infinite"), true, false, false, true);
}

bool UChopItBootstrapCommandlet::CreateEnemyAssets() const
{
	auto CreateEnemy = [](const TCHAR* PackageName, const TCHAR* AssetName, const TCHAR* Id, const TCHAR* Name,
		const float Health, const float Speed, const float Damage, const float Interval, const int32 Experience)
	{
		UChopItEnemyDefinition* Enemy = ChopItBootstrap::LoadOrCreateAsset<UChopItEnemyDefinition>(PackageName, AssetName);
		if (!Enemy) { return static_cast<UChopItEnemyDefinition*>(nullptr); }
		Enemy->EnemyId = FName(Id);
		Enemy->DisplayName = FText::FromString(Name);
		Enemy->MaxHealth = Health;
		Enemy->MoveSpeed = Speed;
		Enemy->ContactDamage = Damage;
		Enemy->AttackInterval = Interval;
		Enemy->AttackRange = 115.0f;
		Enemy->ExperienceReward = Experience;
		Enemy->WoodRewardUnits = 1;
		return ChopItBootstrap::SaveAsset(Enemy) ? Enemy : nullptr;
	};
	UChopItEnemyDefinition* Basic = CreateEnemy(
		TEXT("/Game/ChopIt/AI/Enemies/DA_Enemy_Tree"), TEXT("DA_Enemy_Tree"), TEXT("Tree"), TEXT("Arbol animado"), 48.0f, 250.0f, 7.0f, 1.0f, 6);
	UChopItEnemyDefinition* Fast = CreateEnemy(
		TEXT("/Game/ChopIt/AI/Enemies/DA_Enemy_FastTree"), TEXT("DA_Enemy_FastTree"), TEXT("FastTree"), TEXT("Arbol veloz"), 32.0f, 410.0f, 5.0f, 0.75f, 10);
	if (!Basic || !Fast) { return false; }
	UChopItEnemyDirectorDefinition* Director = ChopItBootstrap::LoadOrCreateAsset<UChopItEnemyDirectorDefinition>(
		TEXT("/Game/ChopIt/AI/Directors/DA_Director_Day01"), TEXT("DA_Director_Day01"));
	if (!Director) { return false; }
	Director->WaveInterval = 2.5f;
	Director->BaseBudget = 3;
	Director->BudgetGrowthPerWave = 1;
	Director->MaxAliveEnemies = 12;
	Director->MinimumSpawnDistance = 700.0f;
	Director->MaximumSpawnDistance = 1450.0f;
	Director->SpawnBoundsExtent = FVector2D(1700.0f, 1700.0f);
	Director->NightEnemies.Reset();
	FChopItEnemySpawnEntry BasicEntry;
	BasicEntry.EnemyClass = AChopItEnemyCharacter::StaticClass();
	BasicEntry.Definition = Basic;
	BasicEntry.BudgetCost = 1;
	BasicEntry.Weight = 0.72f;
	Director->NightEnemies.Add(BasicEntry);
	FChopItEnemySpawnEntry FastEntry;
	FastEntry.EnemyClass = AChopItEnemyCharacter::StaticClass();
	FastEntry.Definition = Fast;
	FastEntry.BudgetCost = 2;
	FastEntry.Weight = 0.28f;
	Director->NightEnemies.Add(FastEntry);
	const bool bSaved = ChopItBootstrap::SaveAsset(Director);
	UE_LOG(LogChopIt, Display, TEXT("Phase 8 enemy definitions and Day 1 director: %s"), bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}

bool UChopItBootstrapCommandlet::CreateProgressionAssets() const
{
	UCurveFloat* Curve = ChopItBootstrap::LoadOrCreateAsset<UCurveFloat>(
		TEXT("/Game/ChopIt/Progression/Curves/Curve_XP_Levels"),
		TEXT("Curve_XP_Levels"));
	if (!Curve)
	{
		return false;
	}
	Curve->FloatCurve.Reset();
	const float Requirements[] = { 10.0f, 18.0f, 28.0f, 40.0f, 55.0f, 72.0f, 92.0f, 115.0f, 140.0f, 170.0f };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Requirements); ++Index)
	{
		Curve->FloatCurve.AddKey(static_cast<float>(Index + 1), Requirements[Index]);
	}
	if (!ChopItBootstrap::SaveAsset(Curve))
	{
		return false;
	}

	struct FUpgradeSpec
	{
		const TCHAR* Name;
		const TCHAR* DisplayName;
		const TCHAR* Description;
		EChopItUpgradeRarity Rarity;
		int32 MaxStacks;
		float Weight;
		TArray<FChopItStatModifier> Modifiers;
	};
	auto Modifier = [](const EChopItCombatStat Stat, const EChopItModifierOperation Operation, const float Magnitude)
	{
		FChopItStatModifier Result;
		Result.Stat = Stat;
		Result.Operation = Operation;
		Result.Magnitude = Magnitude;
		return Result;
	};
	const TArray<FUpgradeSpec> Specs =
	{
		{ TEXT("Filo"), TEXT("Filo afilado"), TEXT("+20% dano del hacha"), EChopItUpgradeRarity::Common, 5, 1.0f,
			{ Modifier(EChopItCombatStat::Damage, EChopItModifierOperation::Multiply, 1.20f) } },
		{ TEXT("Ritmo"), TEXT("Brazos incansables"), TEXT("+15% velocidad de ataque"), EChopItUpgradeRarity::Common, 5, 1.0f,
			{ Modifier(EChopItCombatStat::AttackSpeed, EChopItModifierOperation::Multiply, 1.15f) } },
		{ TEXT("Alcance"), TEXT("Mango extensible"), TEXT("+18% alcance"), EChopItUpgradeRarity::Common, 5, 1.0f,
			{ Modifier(EChopItCombatStat::Range, EChopItModifierOperation::Multiply, 1.18f) } },
		{ TEXT("Critico"), TEXT("Ojo de lenador"), TEXT("+8% probabilidad critica"), EChopItUpgradeRarity::Common, 5, 0.9f,
			{ Modifier(EChopItCombatStat::CriticalChance, EChopItModifierOperation::Add, 0.08f) } },
		{ TEXT("Botas"), TEXT("Botas aceitadas"), TEXT("+12% velocidad de movimiento"), EChopItUpgradeRarity::Common, 5, 0.9f,
			{ Modifier(EChopItCombatStat::MovementSpeed, EChopItModifierOperation::Multiply, 1.12f) } },
		{ TEXT("Furia"), TEXT("Furia torpe"), TEXT("+35% dano, -10% velocidad de ataque"), EChopItUpgradeRarity::Rare, 3, 0.35f,
			{ Modifier(EChopItCombatStat::Damage, EChopItModifierOperation::Multiply, 1.35f), Modifier(EChopItCombatStat::AttackSpeed, EChopItModifierOperation::Multiply, 0.90f) } },
		{ TEXT("Precision"), TEXT("Compas del bosque"), TEXT("+12% critico y +10% alcance"), EChopItUpgradeRarity::Uncommon, 3, 0.6f,
			{ Modifier(EChopItCombatStat::CriticalChance, EChopItModifierOperation::Add, 0.12f), Modifier(EChopItCombatStat::Range, EChopItModifierOperation::Multiply, 1.10f) } },
		{ TEXT("Gigante"), TEXT("Hacha gigante"), TEXT("+25% dano y alcance, -10% cadencia"), EChopItUpgradeRarity::Rare, 3, 0.35f,
			{ Modifier(EChopItCombatStat::Damage, EChopItModifierOperation::Multiply, 1.25f), Modifier(EChopItCombatStat::Range, EChopItModifierOperation::Multiply, 1.25f), Modifier(EChopItCombatStat::AttackSpeed, EChopItModifierOperation::Multiply, 0.90f) } }
	};

	for (const FUpgradeSpec& Spec : Specs)
	{
		const FString AssetName = FString::Printf(TEXT("DA_Upgrade_%s"), Spec.Name);
		const FString Package = FString::Printf(TEXT("/Game/ChopIt/Progression/Upgrades/%s"), *AssetName);
		UChopItUpgradeDefinition* Upgrade = ChopItBootstrap::LoadOrCreateAsset<UChopItUpgradeDefinition>(Package, FName(*AssetName));
		if (!Upgrade)
		{
			return false;
		}
		Upgrade->UpgradeId = FName(Spec.Name);
		Upgrade->DisplayName = FText::FromString(Spec.DisplayName);
		Upgrade->Description = FText::FromString(Spec.Description);
		Upgrade->Rarity = Spec.Rarity;
		Upgrade->MaxStacks = Spec.MaxStacks;
		Upgrade->OfferWeight = Spec.Weight;
		Upgrade->Modifiers = Spec.Modifiers;
		if (!ChopItBootstrap::SaveAsset(Upgrade))
		{
			return false;
		}
	}
	UE_LOG(LogChopIt, Display, TEXT("Phase 6 XP curve and 8 upgrades: OK"));
	return true;
}

bool UChopItBootstrapCommandlet::CreateDayDefinition() const
{
	UChopItDayDefinition* Day = ChopItBootstrap::LoadOrCreateAsset<UChopItDayDefinition>(
		ChopItBootstrap::DayOnePackage,
		TEXT("DA_Day_01"));
	if (!Day)
	{
		return false;
	}
	Day->DayNumber = 1;
	Day->WoodQuota = 3;
	Day->MoneyPerWood = 4;
	Day->TransferBatchSize = 1;
	Day->TransferInterval = 0.15f;
	Day->DayDuration = 30.0f;
	Day->DuskMinimumDuration = 3.0f;
	Day->DuskHardDeadline = 15.0f;
	Day->NightMinimumDuration = 20.0f;
	Day->ElitePlaceholderDuration = 3.0f;
	Day->ResolutionDuration = 2.0f;
	return ChopItBootstrap::SaveAsset(Day);
}

bool UChopItBootstrapCommandlet::CreateEconomyBlueprints() const
{
	struct FBlueprintSpec
	{
		const TCHAR* ObjectPath;
		const TCHAR* PackagePath;
		const TCHAR* AssetName;
		UClass* ParentClass;
	};
	const FBlueprintSpec Specs[] =
	{
		{ TEXT("/Game/ChopIt/World/Economy/BP_CabinHub.BP_CabinHub"), ChopItBootstrap::CabinBlueprintPackage, TEXT("BP_CabinHub"), AChopItCabinHub::StaticClass() },
		{ TEXT("/Game/ChopIt/World/Economy/BP_QuotaMachine.BP_QuotaMachine"), ChopItBootstrap::QuotaMachineBlueprintPackage, TEXT("BP_QuotaMachine"), AChopItQuotaMachine::StaticClass() },
		{ TEXT("/Game/ChopIt/World/Economy/BP_DeliveryZone.BP_DeliveryZone"), ChopItBootstrap::DeliveryZoneBlueprintPackage, TEXT("BP_DeliveryZone"), AChopItDeliveryZone::StaticClass() },
		{ TEXT("/Game/ChopIt/World/Economy/BP_SellZone.BP_SellZone"), ChopItBootstrap::SellZoneBlueprintPackage, TEXT("BP_SellZone"), AChopItSellZone::StaticClass() }
	};

	for (const FBlueprintSpec& Spec : Specs)
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, Spec.ObjectPath);
		if (!Blueprint)
		{
			UPackage* Package = CreatePackage(Spec.PackagePath);
			Blueprint = FKismetEditorUtilities::CreateBlueprint(
				Spec.ParentClass, Package, FName(Spec.AssetName), BPTYPE_Normal, NAME_None);
			if (Blueprint)
			{
				FAssetRegistryModule::AssetCreated(Blueprint);
			}
		}
		if (!Blueprint)
		{
			return false;
		}
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (!ChopItBootstrap::SaveAsset(Blueprint))
		{
			return false;
		}
	}
	UE_LOG(LogChopIt, Display, TEXT("Phase 4 economy Blueprints: OK"));
	return true;
}

bool UChopItBootstrapCommandlet::CreateChainLabAssets() const
{
	if (!CreateBlockoutMaterials())
	{
		return false;
	}
	UChopItChainDefinition* DefaultChainDefinition = ChopItBootstrap::LoadOrCreateAsset<UChopItChainDefinition>(
		ChopItBootstrap::DefaultChainDefinitionPackage, TEXT("DA_Chain_Default"));
	if (!DefaultChainDefinition)
	{
		return false;
	}
	if (!DefaultChainDefinition->ChainLinkMesh)
	{
		DefaultChainDefinition->ChainLinkMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	}
	// The floor remains frictionless while obstacle contact gets enough lateral
	// friction to preserve a wrap. These are the stable custom-rope defaults.
	DefaultChainDefinition->CableSolverIterations = 32;
	DefaultChainDefinition->CableConstraintVelocityDamping = 0.9f;
	DefaultChainDefinition->CableMaximumSubsteps = 8;
	DefaultChainDefinition->CableVelocityDamping = 0.02f;
	DefaultChainDefinition->CableCollisionSkin = 1.0f;
	DefaultChainDefinition->CableCollisionIterations = 5;
	DefaultChainDefinition->CableGroundFriction = 0.0f;
	DefaultChainDefinition->CableCollisionFriction = 0.08f;
	DefaultChainDefinition->WrapSweepRadius = 6.0f;
	DefaultChainDefinition->WrapAnchorSurfaceOffset = 0.75f;
	DefaultChainDefinition->WrapMinimumAnchorSeparation = 12.0f;
	DefaultChainDefinition->MaximumWrapAnchors = 32;
	DefaultChainDefinition->MaximumWrapInsertionsPerFrame = 4;
	DefaultChainDefinition->UnwrapConfirmationFrames = 2;
	DefaultChainDefinition->CableXPBDStiffness = 1.0f;
	DefaultChainDefinition->MinimumVisualParticlesPerSpan = 5;
	DefaultChainDefinition->TensionSoftBand = 30.0f;
	DefaultChainDefinition->PlayerPullAcceleration = 1800.0f;
	DefaultChainDefinition->PlayerPullDamping = 8.0f;
	DefaultChainDefinition->MaximumPropTensionForce = 250000.0f;
	DefaultChainDefinition->PhysicsPropForceScale = 1.0f;
	if (!DefaultChainDefinition->ChainLinkMesh || !ChopItBootstrap::SaveAsset(DefaultChainDefinition))
	{
		return false;
	}

	UBlueprint* ChainLabMachineBlueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/Game/ChopIt/World/ChainLab/BP_ChainLabMachine.BP_ChainLabMachine"));
	if (!ChainLabMachineBlueprint)
	{
		UPackage* Package = CreatePackage(ChopItBootstrap::ChainLabMachineBlueprintPackage);
		ChainLabMachineBlueprint = FKismetEditorUtilities::CreateBlueprint(
			AChopItQuotaMachine::StaticClass(), Package, TEXT("BP_ChainLabMachine"), BPTYPE_Normal, NAME_None);
		if (ChainLabMachineBlueprint)
		{
			FAssetRegistryModule::AssetCreated(ChainLabMachineBlueprint);
		}
	}
	if (!ChainLabMachineBlueprint)
	{
		return false;
	}

	FKismetEditorUtilities::CompileBlueprint(ChainLabMachineBlueprint);
	AChopItQuotaMachine* MachineDefaults = Cast<AChopItQuotaMachine>(ChainLabMachineBlueprint->GeneratedClass
		? ChainLabMachineBlueprint->GeneratedClass->GetDefaultObject()
		: nullptr);
	if (!MachineDefaults)
	{
		return false;
	}
	MachineDefaults->SetChainDefinition(DefaultChainDefinition);
	const bool bSaved = ChopItBootstrap::SaveAsset(ChainLabMachineBlueprint) && RebuildChainLabMap();
	UE_LOG(LogChopIt, Display, TEXT("Chain Lab Blueprint and test map: %s"), bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}

bool UChopItBootstrapCommandlet::CreateInputAssets() const
{
	UInputAction* MoveAction = ChopItBootstrap::LoadOrCreateAsset<UInputAction>(
		ChopItBootstrap::MoveActionPackage, TEXT("IA_Move"));
	UInputAction* InteractAction = ChopItBootstrap::LoadOrCreateAsset<UInputAction>(
		ChopItBootstrap::InteractActionPackage, TEXT("IA_Interact"));
	UInputMappingContext* GameplayContext = ChopItBootstrap::LoadOrCreateAsset<UInputMappingContext>(
		ChopItBootstrap::GameplayContextPackage, TEXT("IMC_Gameplay"));
	if (!MoveAction || !InteractAction || !GameplayContext)
	{
		return false;
	}

	MoveAction->ValueType = EInputActionValueType::Axis2D;
	InteractAction->ValueType = EInputActionValueType::Boolean;
	GameplayContext->UnmapAll();

	auto AddSwizzle = [GameplayContext](FEnhancedActionKeyMapping& Mapping)
	{
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(GameplayContext);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		Mapping.Modifiers.Add(Swizzle);
	};
	auto AddNegate = [GameplayContext](FEnhancedActionKeyMapping& Mapping, bool bX, bool bY)
	{
		UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(GameplayContext);
		Negate->bX = bX;
		Negate->bY = bY;
		Negate->bZ = false;
		Mapping.Modifiers.Add(Negate);
	};

	GameplayContext->MapKey(MoveAction, EKeys::D);
	FEnhancedActionKeyMapping& Left = GameplayContext->MapKey(MoveAction, EKeys::A);
	AddNegate(Left, true, false);
	FEnhancedActionKeyMapping& Forward = GameplayContext->MapKey(MoveAction, EKeys::W);
	AddSwizzle(Forward);
	FEnhancedActionKeyMapping& Backward = GameplayContext->MapKey(MoveAction, EKeys::S);
	AddSwizzle(Backward);
	AddNegate(Backward, false, true);
	GameplayContext->MapKey(MoveAction, EKeys::Gamepad_Left2D);
	GameplayContext->MapKey(InteractAction, EKeys::E);
	GameplayContext->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Bottom);

	const bool bSaved = ChopItBootstrap::SaveAsset(MoveAction)
		&& ChopItBootstrap::SaveAsset(InteractAction)
		&& ChopItBootstrap::SaveAsset(GameplayContext);
	UE_LOG(LogChopIt, Display, TEXT("Phase 1 Enhanced Input assets: %s"), bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}

bool UChopItBootstrapCommandlet::CreateCharacterBlueprint() const
{
	UBlueprint* CharacterBlueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/Game/ChopIt/Characters/Blueprints/BP_ChopItCharacter.BP_ChopItCharacter"));
	if (!CharacterBlueprint)
	{
		UPackage* Package = CreatePackage(ChopItBootstrap::CharacterBlueprintPackage);
		CharacterBlueprint = FKismetEditorUtilities::CreateBlueprint(
			AChopItCharacter::StaticClass(),
			Package,
			TEXT("BP_ChopItCharacter"),
			BPTYPE_Normal,
			NAME_None);
		if (CharacterBlueprint)
		{
			FAssetRegistryModule::AssetCreated(CharacterBlueprint);
		}
	}

	if (!CharacterBlueprint)
	{
		return false;
	}

	FKismetEditorUtilities::CompileBlueprint(CharacterBlueprint);
	const bool bSaved = ChopItBootstrap::SaveAsset(CharacterBlueprint);
	UE_LOG(LogChopIt, Display, TEXT("Phase 1 character Blueprint: %s"), bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}

bool UChopItBootstrapCommandlet::CreateBlockoutMaterials() const
{
	UMaterialInterface* Parent = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Parent)
	{
		return false;
	}

	struct FMaterialSpec
	{
		const TCHAR* Name;
		FLinearColor Color;
	};
	const FMaterialSpec Specs[] =
	{
		{ TEXT("MI_Ground"), FLinearColor(0.055f, 0.20f, 0.07f) },
		{ TEXT("MI_Path"), FLinearColor(0.30f, 0.18f, 0.08f) },
		{ TEXT("MI_Wood"), FLinearColor(0.22f, 0.09f, 0.025f) },
		{ TEXT("MI_Leaves"), FLinearColor(0.015f, 0.12f, 0.025f) },
		{ TEXT("MI_Roof"), FLinearColor(0.24f, 0.025f, 0.02f) },
		{ TEXT("MI_Stone"), FLinearColor(0.18f, 0.21f, 0.22f) },
		{ TEXT("MI_Player"), FLinearColor(0.95f, 0.32f, 0.03f) }
	};

	for (const FMaterialSpec& Spec : Specs)
	{
		const FString PackageName = FString::Printf(
			TEXT("/Game/ChopIt/World/Blockout/Materials/%s"), Spec.Name);
		UMaterialInstanceConstant* Material = ChopItBootstrap::LoadOrCreateAsset<UMaterialInstanceConstant>(
			PackageName, FName(Spec.Name));
		Material->SetParentEditorOnly(Parent);
		Material->SetVectorParameterValueEditorOnly(TEXT("Color"), Spec.Color);
		if (!ChopItBootstrap::SaveAsset(Material))
		{
			return false;
		}
	}
	return true;
}

bool UChopItBootstrapCommandlet::RebuildPhase1Map(
	const FString& LongPackageName,
	const bool bFullSandbox,
	const bool bIncludeCombatDummies,
	const bool bHarvestTestLayout,
	const bool bEconomyTestLayout) const
{
	UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
	if (!World)
	{
		return false;
	}

	World->GetWorldSettings()->DefaultGameMode = AChopItGameMode::StaticClass();

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (!Cube || !Cylinder || !Sphere || !Cone)
	{
		return false;
	}

	UMaterialInterface* Ground = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Ground"));
	UMaterialInterface* Path = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Path"));
	UMaterialInterface* Wood = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Wood"));
	UMaterialInterface* Leaves = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Leaves"));
	UMaterialInterface* Roof = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Roof"));
	UMaterialInterface* Stone = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Stone"));

	ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("Ground"), FVector(0, 0, -50), FVector(42, 42, 1), Ground);
	ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("Hub_Path"), FVector(450, 0, 2), FVector(9, 2.5f, 0.08f), Path, TEXT("NoCollision"));
	if (!bEconomyTestLayout)
	{
		ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("Cabin_Body"), FVector(-250, 0, 200), FVector(5, 4, 4), Wood);
		ChopItBootstrap::SpawnBlockoutMesh(World, Cone, TEXT("Cabin_Roof"), FVector(-250, 0, 520), FVector(4.4f, 4.4f, 2.2f), Roof, TEXT("BlockAll"), FRotator(0, 45, 0));
		ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("Cabin_Door"), FVector(5, 0, 130), FVector(0.15f, 1.1f, 2.6f), Stone);
	}
	ChopItBootstrap::SpawnBlockoutMesh(World, Cylinder, TEXT("Hub_Marker"), FVector(450, 0, 35), FVector(1.5f, 1.5f, 0.7f), Stone, TEXT("NoCollision"));

	const int32 TreeCount = bFullSandbox ? 20 : 12;
	for (int32 Index = 0; Index < TreeCount; ++Index)
	{
		const float Angle = 2.0f * PI * static_cast<float>(Index) / static_cast<float>(TreeCount);
		const float Radius = (bHarvestTestLayout || bEconomyTestLayout) && Index == 0
			? 1000.0f
			: 1550.0f + (Index % 3) * 170.0f;
		FVector TreeBase(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
		if (bEconomyTestLayout && Index < 4)
		{
			const FVector EconomyTreeLocations[] =
			{
				FVector(1000, 0, 0),
				FVector(1100, 350, 0),
				FVector(1100, -350, 0),
				FVector(1400, 0, 0)
			};
			TreeBase = EconomyTreeLocations[Index];
		}
		FActorSpawnParameters TreeSpawnParameters;
		TreeSpawnParameters.Name = FName(*FString::Printf(TEXT("Tree_%02d"), Index));
		AChopItTree* Tree = World->SpawnActor<AChopItTree>(TreeBase + FVector(0, 0, 380), FRotator::ZeroRotator, TreeSpawnParameters);
		if (!Tree)
		{
			return false;
		}
		Tree->SetActorLabel(TreeSpawnParameters.Name.ToString());
		Tree->SetBlockoutMaterials(Wood, Leaves);
	}

	const struct
	{
		FName Name;
		FVector Location;
		FVector Scale;
	} Boundaries[] =
	{
		{ TEXT("Boundary_North"), FVector(0, 2100, 200), FVector(42, 0.25f, 5) },
		{ TEXT("Boundary_South"), FVector(0, -2100, 200), FVector(42, 0.25f, 5) },
		{ TEXT("Boundary_East"), FVector(2100, 0, 200), FVector(0.25f, 42, 5) },
		{ TEXT("Boundary_West"), FVector(-2100, 0, 200), FVector(0.25f, 42, 5) }
	};
	for (const auto& Boundary : Boundaries)
	{
		AStaticMeshActor* BoundaryActor = ChopItBootstrap::SpawnBlockoutMesh(
			World, Cube, Boundary.Name, Boundary.Location, Boundary.Scale, Stone);
		BoundaryActor->SetActorHiddenInGame(true);
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = TEXT("PlayerStart");
	APlayerStart* PlayerStart = World->SpawnActor<APlayerStart>(FVector(750, 0, 100), FRotator(0, 0, 0), SpawnParameters);
	PlayerStart->SetActorLabel(TEXT("PlayerStart"));

	if (bIncludeCombatDummies)
	{
		const FVector DummyLocations[] =
		{
			FVector(1100, 0, 75),
			FVector(1250, -150, 75),
			FVector(650, 650, 75),
			FVector(200, -600, 75)
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(DummyLocations); ++Index)
		{
			SpawnParameters.Name = FName(*FString::Printf(TEXT("CombatDummy_%02d"), Index));
			AChopItCombatDummy* Dummy = World->SpawnActor<AChopItCombatDummy>(
				DummyLocations[Index],
				FRotator::ZeroRotator,
				SpawnParameters);
			if (!Dummy)
			{
				return false;
			}
			Dummy->SetActorLabel(SpawnParameters.Name.ToString());
		}
	}

	if (bEconomyTestLayout)
	{
		SpawnParameters.Name = TEXT("CabinHub");
		AChopItCabinHub* Cabin = World->SpawnActor<AChopItCabinHub>(FVector(-250, 0, 0), FRotator::ZeroRotator, SpawnParameters);
		SpawnParameters.Name = TEXT("QuotaMachine");
		AChopItQuotaMachine* Machine = World->SpawnActor<AChopItQuotaMachine>(FVector(150, -500, 0), FRotator::ZeroRotator, SpawnParameters);
		SpawnParameters.Name = TEXT("DeliveryZone");
		AChopItDeliveryZone* Delivery = World->SpawnActor<AChopItDeliveryZone>(FVector(400, -300, 15), FRotator::ZeroRotator, SpawnParameters);
		SpawnParameters.Name = TEXT("SellZone");
		AChopItSellZone* Sell = World->SpawnActor<AChopItSellZone>(FVector(400, 350, 15), FRotator::ZeroRotator, SpawnParameters);
		SpawnParameters.Name = TEXT("ShopTerminal");
		AChopItShopTerminal* Shop = World->SpawnActor<AChopItShopTerminal>(FVector(-100, 460, 0), FRotator::ZeroRotator, SpawnParameters);
		if (!Cabin || !Machine || !Delivery || !Sell || !Shop)
		{
			return false;
		}
		Cabin->SetActorLabel(TEXT("CabinHub"));
		Machine->SetActorLabel(TEXT("QuotaMachine"));
		Delivery->SetActorLabel(TEXT("DeliveryZone_Quota"));
		Sell->SetActorLabel(TEXT("SellZone_Truck"));
		Shop->SetActorLabel(TEXT("ShopTerminal"));
	}

	SpawnParameters.Name = TEXT("DirectionalLight");
	ADirectionalLight* DirectionalLight = World->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-55, -35, 0), SpawnParameters);
	DirectionalLight->SetActorLabel(TEXT("DirectionalLight"));
	DirectionalLight->GetLightComponent()->SetIntensity(5.0f);
	DirectionalLight->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.88f, 0.70f));
	DirectionalLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);

	SpawnParameters.Name = TEXT("SkyLight");
	ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	SkyLight->SetActorLabel(TEXT("SkyLight"));
	SkyLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	SkyLight->GetLightComponent()->SetIntensity(1.2f);
	SkyLight->GetLightComponent()->SetRealTimeCapture(true);

	SpawnParameters.Name = TEXT("SkyAtmosphere");
	ASkyAtmosphere* SkyAtmosphere = World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	SkyAtmosphere->SetActorLabel(TEXT("SkyAtmosphere"));

	SpawnParameters.Name = TEXT("HeightFog");
	AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(FVector(0, 0, -50), FRotator::ZeroRotator, SpawnParameters);
	Fog->SetActorLabel(TEXT("HeightFog"));

	SpawnParameters.Name = TEXT("NavMeshBounds");
	ANavMeshBoundsVolume* NavBounds = World->SpawnActor<ANavMeshBoundsVolume>(FVector(0, 0, 200), FRotator::ZeroRotator, SpawnParameters);
	NavBounds->SetActorLabel(TEXT("NavMeshBounds"));
	NavBounds->SetActorScale3D(FVector(22, 22, 5));

	const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, LongPackageName);
	UE_LOG(LogChopIt, Display, TEXT("Rebuilt Phase 1 map %s: %s"), *LongPackageName, bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}

bool UChopItBootstrapCommandlet::RebuildChainLabMap() const
{
	UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
	if (!World)
	{
		return false;
	}
	World->GetWorldSettings()->DefaultGameMode = AChopItGameMode::StaticClass();

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!Cube || !Cylinder || !Sphere)
	{
		return false;
	}

	UMaterialInterface* Ground = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Ground"));
	UMaterialInterface* Path = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Path"));
	UMaterialInterface* Wood = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Wood"));
	UMaterialInterface* Leaves = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Leaves"));
	UMaterialInterface* Stone = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Stone"));
	if (!Ground || !Path || !Wood || !Leaves || !Stone)
	{
		return false;
	}

	// All obstacle meshes use BlockAll. The ChopItChain profile therefore sweeps
	// against them exactly like it will against normal world geometry in game.
	ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("ChainLab_Ground"), FVector(0, 0, -50), FVector(50, 50, 1), Ground);
	ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("ChainLab_StartPath"), FVector(0, -1500, 3), FVector(3.0f, 8.0f, 0.08f), Path, TEXT("NoCollision"));

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = TEXT("PlayerStart_ChainLab");
	APlayerStart* PlayerStart = World->SpawnActor<APlayerStart>(FVector(0, -1900, 110), FRotator::ZeroRotator, SpawnParameters);
	if (!PlayerStart)
	{
		return false;
	}
	PlayerStart->SetActorLabel(TEXT("PlayerStart_ChainLab"));

	UBlueprint* ChainLabBlueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/Game/ChopIt/World/ChainLab/BP_ChainLabMachine.BP_ChainLabMachine"));
	UClass* MachineClass = AChopItQuotaMachine::StaticClass();
	if (ChainLabBlueprint && ChainLabBlueprint->GeneratedClass)
	{
		MachineClass = ChainLabBlueprint->GeneratedClass;
	}
	SpawnParameters.Name = TEXT("BP_ChainLabMachine");
	AChopItQuotaMachine* Machine = World->SpawnActor<AChopItQuotaMachine>(
		MachineClass, FVector(0, -1200, 0), FRotator::ZeroRotator, SpawnParameters);
	if (!Machine)
	{
		return false;
	}
	Machine->SetActorLabel(TEXT("BP_ChainLabMachine (Edit Chain Settings Here)"));
	ChopItBootstrap::SpawnLabSign(World, TEXT("Sign_ChainLabTitle"), TEXT("CHAIN LAB - MACHINE SETTINGS IN BP_ChainLabMachine"), FVector(0, -1110, 340));

	// 01: wide, spaced poles. This catches the most common wrapping and release cases.
	const FVector WrapPillars[] =
	{
		FVector(-340, -830, 210), FVector(0, -700, 210), FVector(360, -830, 210),
		FVector(-180, -390, 210), FVector(260, -310, 210)
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(WrapPillars); ++Index)
	{
		ChopItBootstrap::SpawnBlockoutMesh(
			World, Cylinder, FName(*FString::Printf(TEXT("WrapPillar_%02d"), Index)), WrapPillars[Index], FVector(1.4f, 1.4f, 4.2f), Wood);
	}
	ChopItBootstrap::SpawnLabSign(World, TEXT("Sign_WrapPillars"), TEXT("01  WRAP / UNWRAP POSTS"), FVector(0, -520, 470));

	// 02: a narrow slalom and alternating wall corners exercise particle density.
	const FVector SlalomPosts[] =
	{
		FVector(720, -920, 160), FVector(980, -720, 160), FVector(720, -520, 160), FVector(980, -320, 160),
		FVector(720, -120, 160), FVector(980, 80, 160)
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(SlalomPosts); ++Index)
	{
		ChopItBootstrap::SpawnBlockoutMesh(
			World, Cylinder, FName(*FString::Printf(TEXT("SlalomPost_%02d"), Index)), SlalomPosts[Index], FVector(0.85f, 0.85f, 3.2f), Stone);
	}
	ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("CornerWall_A"), FVector(1320, -520, 125), FVector(0.35f, 4.4f, 2.5f), Stone);
	ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("CornerWall_B"), FVector(1130, -100, 125), FVector(2.2f, 0.35f, 2.5f), Stone);
	ChopItBootstrap::SpawnLabSign(World, TEXT("Sign_Slalom"), TEXT("02  DENSE SLALOM + CORNERS"), FVector(900, 240, 390));

	// 03: a frame and an overhang make the chain pass under, around and over level pieces.
	ChopItBootstrap::SpawnBlockoutMesh(World, Cylinder, TEXT("FramePost_Left"), FVector(-730, -90, 210), FVector(1.0f, 1.0f, 4.2f), Wood);
	ChopItBootstrap::SpawnBlockoutMesh(World, Cylinder, TEXT("FramePost_Right"), FVector(-170, -90, 210), FVector(1.0f, 1.0f, 4.2f), Wood);
	ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("FrameTopBeam"), FVector(-450, -90, 405), FVector(3.4f, 0.6f, 0.45f), Wood);
	ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("Overhang"), FVector(-480, 330, 300), FVector(5.0f, 1.2f, 0.35f), Stone);
	ChopItBootstrap::SpawnBlockoutMesh(World, Cylinder, TEXT("OverhangSupport_A"), FVector(-880, 330, 150), FVector(0.55f, 0.55f, 3.0f), Stone);
	ChopItBootstrap::SpawnBlockoutMesh(World, Cylinder, TEXT("OverhangSupport_B"), FVector(-80, 330, 150), FVector(0.55f, 0.55f, 3.0f), Stone);
	ChopItBootstrap::SpawnLabSign(World, TEXT("Sign_Frame"), TEXT("03  FRAME + OVERHANG"), FVector(-460, 560, 470));

	// 04: ramps and tiered cubes exercise sliding collision without floor snagging.
	ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("Ramp_Up"), FVector(450, 520, 105), FVector(5.6f, 3.0f, 0.35f), Path, TEXT("BlockAll"), FRotator(16.0f, 0.0f, 0.0f));
	ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("Ramp_Down"), FVector(1040, 520, 105), FVector(5.6f, 3.0f, 0.35f), Path, TEXT("BlockAll"), FRotator(-16.0f, 0.0f, 0.0f));
	ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("TierBlock_Low"), FVector(820, 920, 75), FVector(2.1f, 2.1f, 1.5f), Stone);
	ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("TierBlock_High"), FVector(1120, 920, 150), FVector(1.8f, 1.8f, 3.0f), Stone);
	ChopItBootstrap::SpawnLabSign(World, TEXT("Sign_Ramps"), TEXT("04  RAMPS + TIERS"), FVector(760, 1210, 420));

	// Movable prop: authoritative tension should pull this box at its wrap point,
	// while visual particles remain collision-only and never add a second force.
	AStaticMeshActor* PhysicsBox = ChopItBootstrap::SpawnBlockoutMesh(
		World, Cube, TEXT("PhysicsProp_TetherPull"), FVector(1450, 760, 90), FVector(1.4f, 1.4f, 1.4f), Stone);
	if (PhysicsBox && PhysicsBox->GetStaticMeshComponent())
	{
		UStaticMeshComponent* Mesh = PhysicsBox->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetSimulatePhysics(true);
		Mesh->SetMassOverrideInKg(NAME_None, 50.0f, true);
	}
	ChopItBootstrap::SpawnLabSign(World, TEXT("Sign_PhysicsProp"), TEXT("PHYSICS PROP - WRAP AND PULL"), FVector(1450, 980, 330));

	// 05: trunks with collision and harmless canopies recreate the important tree case.
	const FVector TreeLocations[] = { FVector(-1180, 820, 230), FVector(-1540, 520, 230), FVector(-1450, 1110, 230) };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(TreeLocations); ++Index)
	{
		const FVector TrunkLocation = TreeLocations[Index];
		ChopItBootstrap::SpawnBlockoutMesh(World, Cylinder, FName(*FString::Printf(TEXT("ChainTreeTrunk_%02d"), Index)), TrunkLocation, FVector(1.25f, 1.25f, 4.6f), Wood);
		ChopItBootstrap::SpawnBlockoutMesh(World, Sphere, FName(*FString::Printf(TEXT("ChainTreeCanopy_%02d"), Index)), TrunkLocation + FVector(0, 0, 290), FVector(3.4f, 3.4f, 2.4f), Leaves, TEXT("NoCollision"));
	}
	ChopItBootstrap::SpawnLabSign(World, TEXT("Sign_Trees"), TEXT("05  TREE TRUNKS"), FVector(-1360, 1420, 490));

	const struct { FName Name; FVector Location; FVector Scale; } Boundaries[] =
	{
		{ TEXT("ChainLab_BoundaryNorth"), FVector(0, 2500, 220), FVector(50, 0.25f, 5) },
		{ TEXT("ChainLab_BoundarySouth"), FVector(0, -2500, 220), FVector(50, 0.25f, 5) },
		{ TEXT("ChainLab_BoundaryEast"), FVector(2500, 0, 220), FVector(0.25f, 50, 5) },
		{ TEXT("ChainLab_BoundaryWest"), FVector(-2500, 0, 220), FVector(0.25f, 50, 5) }
	};
	for (const auto& Boundary : Boundaries)
	{
		AStaticMeshActor* Actor = ChopItBootstrap::SpawnBlockoutMesh(World, Cube, Boundary.Name, Boundary.Location, Boundary.Scale, Stone);
		if (Actor)
		{
			Actor->SetActorHiddenInGame(true);
		}
	}

	SpawnParameters.Name = TEXT("DirectionalLight_ChainLab");
	ADirectionalLight* DirectionalLight = World->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-55, -35, 0), SpawnParameters);
	SpawnParameters.Name = TEXT("SkyLight_ChainLab");
	ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	SpawnParameters.Name = TEXT("SkyAtmosphere_ChainLab");
	World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	SpawnParameters.Name = TEXT("HeightFog_ChainLab");
	World->SpawnActor<AExponentialHeightFog>(FVector(0, 0, -50), FRotator::ZeroRotator, SpawnParameters);
	if (DirectionalLight)
	{
		DirectionalLight->SetActorLabel(TEXT("DirectionalLight"));
		DirectionalLight->GetLightComponent()->SetIntensity(5.0f);
		DirectionalLight->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.88f, 0.70f));
		DirectionalLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	}
	if (SkyLight)
	{
		SkyLight->SetActorLabel(TEXT("SkyLight"));
		SkyLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		SkyLight->GetLightComponent()->SetIntensity(1.2f);
		SkyLight->GetLightComponent()->SetRealTimeCapture(true);
	}

	SpawnParameters.Name = TEXT("NavMeshBounds_ChainLab");
	ANavMeshBoundsVolume* NavBounds = World->SpawnActor<ANavMeshBoundsVolume>(FVector(0, 0, 200), FRotator::ZeroRotator, SpawnParameters);
	if (NavBounds)
	{
		NavBounds->SetActorLabel(TEXT("NavMeshBounds"));
		NavBounds->SetActorScale3D(FVector(25, 25, 5));
	}
	const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, ChopItBootstrap::ChainLabMap);
	UE_LOG(LogChopIt, Display, TEXT("Rebuilt Chain Lab %s: %s"), ChopItBootstrap::ChainLabMap, bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}
