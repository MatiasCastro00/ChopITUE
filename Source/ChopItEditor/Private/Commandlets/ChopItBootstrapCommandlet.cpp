#include "Commandlets/ChopItBootstrapCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AI/NavigationSystemBase.h"
#include "ChopItCollision.h"
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
#include "Engine/Texture2D.h"
#include "Economy/ChopItCabinHub.h"
#include "Economy/ChopItChainDefinition.h"
#include "Economy/ChopItDayDefinition.h"
#include "Economy/ChopItDeliveryZone.h"
#include "Economy/ChopItQuotaMachine.h"
#include "Economy/ChopItSellZone.h"
#include "Economy/ChopItWoodGrantZone.h"
#include "Enemies/ChopItEnemyCharacter.h"
#include "Enemies/ChopItEnemyDefinition.h"
#include "GameFramework/WorldSettings.h"
#include "Harvest/ChopItTree.h"
#include "Harvest/ChopItLogPickup.h"
#include "EngineUtils.h"
#include "EnhancedActionKeyMapping.h"
#include "FileHelpers.h"
#include "Framework/ChopItGameMode.h"
#include "Dialogue/ChopItDialogueAssets.h"
#include "Dialogue/ChopItDialogueStageCharacter.h"
#include "Dialogue/ChopItDialogueTrigger.h"
#include "GameFramework/PlayerStart.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialFunctionInterface.h"
#include "MaterialEditingLibrary.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "Player/ChopItCharacter.h"
#include "Progression/ChopItUpgradeDefinition.h"
#include "Shop/ChopItShopTerminal.h"
#include "Spawning/ChopItEnemyDirectorDefinition.h"
#include "StaticMeshCompiler.h"
#include "Pacts/ChopItPactDefinition.h"
#include "Targets/ChopItCombatDummy.h"
#include "UObject/SavePackage.h"
#include "Weapons/ChopItWeaponDefinition.h"
#include "Build/CameraAssetAssembleUtils.h"
#include "Camera/ChopItCameraCue.h"
#include "Camera/ChopItCameraAnchor.h"
#include "Camera/ChopItCameraDemoTrigger.h"
#include "Camera/ChopItCameraStateTreeNodes.h"
#include "Core/CameraAsset.h"
#include "Core/CameraRigAsset.h"
#include "Core/CameraShakeAsset.h"
#include "Directors/CameraDirectorStateTreeSchema.h"
#include "Directors/SingleCameraDirector.h"
#include "Directors/StateTreeCameraDirector.h"
#include "Nodes/Common/FieldOfViewCameraNode.h"
#include "Nodes/Common/PostProcessCameraNode.h"
#include "Nodes/Common/ArrayCameraNode.h"
#include "Nodes/Collision/OcclusionMaterialCameraNode.h"
#include "Nodes/Shakes/EnvelopeShakeCameraNode.h"
#include "Nodes/Shakes/PerlinNoiseLocationShakeCameraNode.h"
#include "StateTree.h"
#include "StateTreeCompiler.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"

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
	constexpr TCHAR DialogueMap[] = TEXT("/Game/ChopIt/World/Maps/L_Test_Dialogue");
	constexpr TCHAR MoveActionPackage[] = TEXT("/Game/ChopIt/Input/IA_Move");
	constexpr TCHAR InteractActionPackage[] = TEXT("/Game/ChopIt/Input/IA_Interact");
	constexpr TCHAR CameraLookActionPackage[] = TEXT("/Game/ChopIt/Input/IA_CameraLook");
	constexpr TCHAR CameraZoomActionPackage[] = TEXT("/Game/ChopIt/Input/IA_CameraZoom");
	constexpr TCHAR CameraResetActionPackage[] = TEXT("/Game/ChopIt/Input/IA_CameraReset");
	constexpr TCHAR GameplayContextPackage[] = TEXT("/Game/ChopIt/Input/IMC_Gameplay");
	constexpr TCHAR DialogueAdvanceActionPackage[] = TEXT("/Game/ChopIt/Input/IA_DialogueAdvance");
	constexpr TCHAR DialogueNextChoiceActionPackage[] = TEXT("/Game/ChopIt/Input/IA_DialogueNextChoice");
	constexpr TCHAR DialoguePreviousChoiceActionPackage[] = TEXT("/Game/ChopIt/Input/IA_DialoguePreviousChoice");
	constexpr TCHAR DialogueCancelActionPackage[] = TEXT("/Game/ChopIt/Input/IA_DialogueCancel");
	constexpr TCHAR DialogueContextPackage[] = TEXT("/Game/ChopIt/Input/IMC_Dialogue");
	constexpr TCHAR CameraAssetPackage[] = TEXT("/Game/ChopIt/Presentation/Camera/CA_PlayerCameras");
	constexpr TCHAR CameraStateTreePackage[] = TEXT("/Game/ChopIt/Presentation/Camera/ST_CameraDirector");
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
	constexpr TCHAR DialogueThemePackage[] = TEXT("/Game/ChopIt/Dialogue/DA_DialogueTheme_WoodMetal");
	constexpr TCHAR BrunaSpeakerPackage[] = TEXT("/Game/ChopIt/Dialogue/Speakers/DA_Speaker_Bruna");
	constexpr TCHAR MiloSpeakerPackage[] = TEXT("/Game/ChopIt/Dialogue/Speakers/DA_Speaker_Milo");
	constexpr TCHAR DialogueDemoPackage[] = TEXT("/Game/ChopIt/Dialogue/Sequences/DA_Dialogue_Demo");
	constexpr TCHAR DeathSpeakerPackage[] = TEXT("/Game/ChopIt/Dialogue/Speakers/DA_Speaker_Death");
	constexpr TCHAR OvenSpeakerPackage[] = TEXT("/Game/ChopIt/Dialogue/Speakers/DA_Speaker_Oven");
	constexpr TCHAR ProtagonistSpeakerPackage[] = TEXT("/Game/ChopIt/Dialogue/Speakers/DA_Speaker_Protagonist");
	constexpr TCHAR MatchIntroDialoguePackage[] = TEXT("/Game/ChopIt/Dialogue/Sequences/DA_Dialogue_MatchIntro");

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

		if (UObject* ExistingObject = FindObject<UObject>(Package, *AssetName.ToString()))
		{
			if (AssetType* ExistingAsset = Cast<AssetType>(ExistingObject)) return ExistingAsset;
			UE_LOG(
				LogChopIt,
				Error,
				TEXT("Refusing to replace %s %s with incompatible class %s."),
				*ExistingObject->GetClass()->GetName(),
				*ExistingObject->GetPathName(),
				*AssetType::StaticClass()->GetName());
			return nullptr;
		}

		AssetType* Asset = NewObject<AssetType>(Package, AssetName, RF_Public | RF_Standalone);
		if (Asset) FAssetRegistryModule::AssetCreated(Asset);
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
		if (!IsValid(World) || !IsValid(Mesh)) return nullptr;
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
		if (!Component)
		{
			Actor->Destroy();
			return nullptr;
		}
		Component->SetStaticMesh(Mesh);
		Component->SetMaterial(0, Material);
		Component->SetMobility(EComponentMobility::Static);
		Component->SetCollisionProfileName(CollisionProfile);
		// Camera collision is opt-in. Ordinary props retain their gameplay collision but
		// never shorten the camera orbit; only authored floor/boundary surfaces override it.
		Component->SetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid, ECR_Ignore);
		return Actor;
	}

	void MarkCameraSolid(AStaticMeshActor* Actor)
	{
		if (!Actor || !Actor->GetStaticMeshComponent()) return;
		Actor->Tags.AddUnique(ChopItCollisionChannels::CameraSolidTag);
		Actor->GetStaticMeshComponent()->ComponentTags.AddUnique(ChopItCollisionChannels::CameraSolidTag);
		Actor->GetStaticMeshComponent()->SetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid, ECR_Block);
	}

	ATextRenderActor* SpawnLabSign(
		UWorld* World,
		const FName Name,
		const FString& Text,
		const FVector& Location,
		const FRotator& Rotation = FRotator(0.0f, 180.0f, 0.0f))
	{
		if (!IsValid(World)) return nullptr;
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = Name;
		ATextRenderActor* Actor = World->SpawnActor<ATextRenderActor>(Location, Rotation, SpawnParameters);
		if (!Actor)
		{
			return nullptr;
		}

		Actor->SetActorLabel(Name.ToString());
		UTextRenderComponent* TextComponent = Actor->GetTextRender();
		if (!TextComponent)
		{
			Actor->Destroy();
			return nullptr;
		}
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
if (FParse::Param(*Params, TEXT("HarvestBlueprints")) && !CreateHarvestBlueprints()) return 1;
if (FParse::Param(*Params, TEXT("Presentation")) && !CreateDamageTextMaterial()) return 1;
if (FParse::Param(*Params, TEXT("Camera")) && !CreateCameraAssets()) return 1;
if (FParse::Param(*Params, TEXT("CameraInput")) && !CreateInputAssets()) return 1;
if (FParse::Param(*Params, TEXT("CameraDemoMap")) && !RebuildPhase1Map(ChopItBootstrap::SandboxMap, true)) return 1;
if (FParse::Param(*Params, TEXT("CriticalBalance"))
	&& (!CreateBasicAxeAsset() || !CreateSharedWeaponAssets())) return 1;
if (FParse::Param(*Params, TEXT("CycleBalance")) && !CreateDayDefinition()) return 1;
if (FParse::Param(*Params, TEXT("RebuildNavMesh"))
	&& !RebuildNavigationData(ChopItBootstrap::ProgressionMap)) return 1;
if (FParse::Param(*Params, TEXT("ChainLab")) && !CreateChainLabAssets())
{
	UE_LOG(LogChopIt, Error, TEXT("Failed to create Chain Lab assets."));
	return 1;
}
if (FParse::Param(*Params, TEXT("Dialogue")) && !CreateDialogueAssets())
{
	UE_LOG(LogChopIt, Error, TEXT("Failed to create Dialogue assets."));
	return 1;
}
if (FParse::Param(*Params, TEXT("DeliveryZones")) && !PlaceDeliveryZonesInStartupMap())
{
	UE_LOG(LogChopIt, Error, TEXT("Failed to place Startup delivery zones."));
	return 1;
}
if (FParse::Param(*Params, TEXT("StartupChainObstacles")) && !PlaceChainObstaclesInStartupMap())
{
	UE_LOG(LogChopIt, Error, TEXT("Failed to place Startup chain obstacles."));
	return 1;
}
// Refresh every player-facing generated text asset without rebuilding or saving
// DA_Day_01. That balance asset is deliberately excluded from this localization pass.
if (FParse::Param(*Params, TEXT("EnglishTextAssets"))
	&& (!CreateBasicAxeAsset()
		|| !CreateSharedWeaponAssets()
		|| !CreateShopBlueprint()
		|| !CreateHarvestBlueprints()
		|| !CreateEconomyBlueprints()
		|| !CreateProgressionAssets()
		|| !CreateEnemyAssets()
		|| !CreatePhase9Assets()
		|| !CreatePhase10Assets()
		|| !CreateDialogueAssets()))
{
	UE_LOG(LogChopIt, Error, TEXT("Failed to refresh the English player-facing assets."));
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
		&& CreateDamageTextMaterial()
		&& CreateInputAssets()
		&& CreateCameraAssets()
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

	BasicAxe->DisplayName = FText::FromString(TEXT("Basic Axe"));
	BasicAxe->Description = FText::FromString(TEXT("The woodcutter's exclusive weapon"));
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
	BasicAxe->CriticalChance = 0.15f;
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
		{ ChopItBootstrap::HandSawPackage, TEXT("DA_Weapon_HandSaw"), TEXT("HandSaw"), TEXT("Hand Chainsaw"), TEXT("Cuts quickly in a short arc"), EChopItWeaponAttackPattern::ArcMelee, 20, 13.0f, 0.22f, 320.0f, 70.0f, 2 },
		{ ChopItBootstrap::SawHaloPackage, TEXT("DA_Weapon_SawHalo"), TEXT("SawHalo"), TEXT("Saw Halo"), TEXT("Strikes in every direction"), EChopItWeaponAttackPattern::RadialMelee, 32, 18.0f, 0.55f, 390.0f, 180.0f, 5 }
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
		Weapon->CriticalChance = 0.15f;
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
	const bool bGuard = Configure(TEXT("/Game/ChopIt/AI/Enemies/DA_Enemy_Guardian"), TEXT("DA_Enemy_Guardian"), TEXT("Guardian"), TEXT("Forest Guardian"), 260.0f, 210.0f, 18.0f, 30);
	const bool bFinal = Configure(TEXT("/Game/ChopIt/AI/Enemies/DA_Enemy_ForestEntity"), TEXT("DA_Enemy_ForestEntity"), TEXT("ForestEntity"), TEXT("Forest Entity"), 520.0f, 260.0f, 24.0f, 75);
	return bGuard && bFinal && RebuildPhase1Map(TEXT("/Game/ChopIt/World/Maps/L_Test_Elite"), true, false, false, true);
}

bool UChopItBootstrapCommandlet::CreatePhase10Assets() const
{
	auto Make=[](const TCHAR* Id,const TCHAR* Name,const TCHAR* Desc,const TArray<FChopItStatModifier>& Modifiers)
	{ FString Asset=FString::Printf(TEXT("DA_Pact_%s"),Id); UChopItPactDefinition* P=ChopItBootstrap::LoadOrCreateAsset<UChopItPactDefinition>(FString::Printf(TEXT("/Game/ChopIt/Pacts/%s"),*Asset),FName(*Asset)); if(!P)return false; P->PactId=FName(Id);P->DisplayName=FText::FromString(Name);P->Description=FText::FromString(Desc);P->CurseIncrease=1;P->Modifiers=Modifiers;return ChopItBootstrap::SaveAsset(P); };
	auto Mod=[](EChopItCombatStat S,float V){ FChopItStatModifier M; M.Stat=S;M.Operation=EChopItModifierOperation::Multiply;M.Magnitude=V;return M; };
	return Make(TEXT("Furia"),TEXT("Reaper's Fury"),TEXT("+35% damage  |  -15% attack speed"),{Mod(EChopItCombatStat::Damage,1.35f),Mod(EChopItCombatStat::AttackSpeed,0.85f)})
		&& Make(TEXT("Ritmo"),TEXT("Funeral Rhythm"),TEXT("+25% attack speed  |  -10% movement speed"),{Mod(EChopItCombatStat::AttackSpeed,1.25f),Mod(EChopItCombatStat::MovementSpeed,0.90f)})
		&& Make(TEXT("Botas"),TEXT("Spectral Step"),TEXT("+20% movement speed  |  -15% range"),{Mod(EChopItCombatStat::MovementSpeed,1.20f),Mod(EChopItCombatStat::Range,0.85f)});
}

bool UChopItBootstrapCommandlet::CreatePhase12Assets() const
{
	return CreatePhase9Assets()
		&& RebuildPhase1Map(TEXT("/Game/ChopIt/World/Maps/L_Test_Infinite"), true, false, false, true);
}

bool UChopItBootstrapCommandlet::CreateDialogueAssets() const
{
	// Dialogue bootstrapping must not rewrite the project's established gameplay input
	// and camera packages. In source-controlled/read-only workspaces those packages may
	// deliberately be locked, while these five dialogue packages are independent.
	UInputAction* DialogueAdvance = ChopItBootstrap::LoadOrCreateAsset<UInputAction>(ChopItBootstrap::DialogueAdvanceActionPackage, TEXT("IA_DialogueAdvance"));
	UInputAction* DialogueNext = ChopItBootstrap::LoadOrCreateAsset<UInputAction>(ChopItBootstrap::DialogueNextChoiceActionPackage, TEXT("IA_DialogueNextChoice"));
	UInputAction* DialoguePrevious = ChopItBootstrap::LoadOrCreateAsset<UInputAction>(ChopItBootstrap::DialoguePreviousChoiceActionPackage, TEXT("IA_DialoguePreviousChoice"));
	UInputAction* DialogueCancel = ChopItBootstrap::LoadOrCreateAsset<UInputAction>(ChopItBootstrap::DialogueCancelActionPackage, TEXT("IA_DialogueCancel"));
	UInputMappingContext* DialogueContext = ChopItBootstrap::LoadOrCreateAsset<UInputMappingContext>(ChopItBootstrap::DialogueContextPackage, TEXT("IMC_Dialogue"));
	if (!DialogueAdvance || !DialogueNext || !DialoguePrevious || !DialogueCancel || !DialogueContext) return false;

	for (UInputAction* DialogueAction : {DialogueAdvance, DialogueNext, DialoguePrevious, DialogueCancel})
	{
		DialogueAction->ValueType = EInputActionValueType::Boolean;
		DialogueAction->bTriggerWhenPaused = true;
		DialogueAction->bConsumeInput = true;
	}
	DialogueContext->UnmapAll();
	DialogueContext->MapKey(DialogueAdvance, EKeys::E);
	DialogueContext->MapKey(DialogueAdvance, EKeys::Enter);
	DialogueContext->MapKey(DialogueAdvance, EKeys::SpaceBar);
	DialogueContext->MapKey(DialogueAdvance, EKeys::Gamepad_FaceButton_Bottom);
	DialogueContext->MapKey(DialogueNext, EKeys::S);
	DialogueContext->MapKey(DialogueNext, EKeys::Down);
	DialogueContext->MapKey(DialogueNext, EKeys::Gamepad_DPad_Down);
	DialogueContext->MapKey(DialoguePrevious, EKeys::W);
	DialogueContext->MapKey(DialoguePrevious, EKeys::Up);
	DialogueContext->MapKey(DialoguePrevious, EKeys::Gamepad_DPad_Up);
	DialogueContext->MapKey(DialogueCancel, EKeys::Escape);
	DialogueContext->MapKey(DialogueCancel, EKeys::Gamepad_FaceButton_Right);
	if (!ChopItBootstrap::SaveAsset(DialogueAdvance)
		|| !ChopItBootstrap::SaveAsset(DialogueNext)
		|| !ChopItBootstrap::SaveAsset(DialoguePrevious)
		|| !ChopItBootstrap::SaveAsset(DialogueCancel)
		|| !ChopItBootstrap::SaveAsset(DialogueContext)) return false;

	const FString PortraitSource = FPaths::Combine(FPaths::ProjectDir(), TEXT("Build/Dialogue/Portraits"));
	const FString PortraitDestination = TEXT("/Game/ChopIt/Dialogue/Portraits");
	const TCHAR* PortraitNames[] =
	{
		TEXT("Bruna_Neutral"), TEXT("Bruna_Angry"), TEXT("Bruna_Surprised"),
		TEXT("Milo_Neutral"), TEXT("Milo_Angry"), TEXT("Milo_Surprised"),
		TEXT("Death_Neutral"), TEXT("Oven_Hungry"), TEXT("Protagonist_Surprised")
	};
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	for (const TCHAR* PortraitName : PortraitNames)
	{
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *PortraitDestination, PortraitName, PortraitName);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		if (!Texture)
		{
			const FString Filename = FPaths::Combine(PortraitSource, FString(PortraitName) + TEXT(".png"));
			if (!FPaths::FileExists(Filename))
			{
				UE_LOG(LogChopIt, Error, TEXT("Missing generated dialogue portrait: %s"), *Filename);
				return false;
			}
			const TArray<UObject*> Imported = AssetToolsModule.Get().ImportAssets({Filename}, PortraitDestination, nullptr, false);
			Texture = Imported.IsEmpty() ? nullptr : Cast<UTexture2D>(Imported[0]);
			if (!Texture) return false;
			Texture->CompressionSettings = TC_EditorIcon;
			Texture->MipGenSettings = TMGS_FromTextureGroup;
			Texture->LODGroup = TEXTUREGROUP_UI;
			Texture->SRGB = true;
			if (!ChopItBootstrap::SaveAsset(Texture)) return false;
		}
	}

	auto Portrait = [PortraitDestination](const TCHAR* Name)
	{
		return LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("%s/%s.%s"), *PortraitDestination, Name, Name));
	};

	UChopItDialogueSpeakerDefinition* Bruna = ChopItBootstrap::LoadOrCreateAsset<UChopItDialogueSpeakerDefinition>(ChopItBootstrap::BrunaSpeakerPackage, TEXT("DA_Speaker_Bruna"));
	UChopItDialogueSpeakerDefinition* Milo = ChopItBootstrap::LoadOrCreateAsset<UChopItDialogueSpeakerDefinition>(ChopItBootstrap::MiloSpeakerPackage, TEXT("DA_Speaker_Milo"));
	UChopItDialogueTheme* Theme = ChopItBootstrap::LoadOrCreateAsset<UChopItDialogueTheme>(ChopItBootstrap::DialogueThemePackage, TEXT("DA_DialogueTheme_WoodMetal"));
	UChopItDialogueSequence* Sequence = ChopItBootstrap::LoadOrCreateAsset<UChopItDialogueSequence>(ChopItBootstrap::DialogueDemoPackage, TEXT("DA_Dialogue_Demo"));
	UChopItDialogueSpeakerDefinition* Death = ChopItBootstrap::LoadOrCreateAsset<UChopItDialogueSpeakerDefinition>(ChopItBootstrap::DeathSpeakerPackage, TEXT("DA_Speaker_Death"));
	UChopItDialogueSpeakerDefinition* Oven = ChopItBootstrap::LoadOrCreateAsset<UChopItDialogueSpeakerDefinition>(ChopItBootstrap::OvenSpeakerPackage, TEXT("DA_Speaker_Oven"));
	UChopItDialogueSpeakerDefinition* Protagonist = ChopItBootstrap::LoadOrCreateAsset<UChopItDialogueSpeakerDefinition>(ChopItBootstrap::ProtagonistSpeakerPackage, TEXT("DA_Speaker_Protagonist"));
	UChopItDialogueSequence* MatchIntro = ChopItBootstrap::LoadOrCreateAsset<UChopItDialogueSequence>(ChopItBootstrap::MatchIntroDialoguePackage, TEXT("DA_Dialogue_MatchIntro"));
	if (!Bruna || !Milo || !Theme || !Sequence || !Death || !Oven || !Protagonist || !MatchIntro) return false;

	Bruna->SpeakerId = TEXT("Bruna");
	Bruna->DisplayName = FText::FromString(TEXT("BRUNA"));
	Bruna->AccentColor = FLinearColor(1.0f, 0.31f, 0.035f, 1.0f);
	Bruna->PortraitSide = EChopItDialoguePortraitSide::Left;
	Bruna->Portraits = {{TEXT("Neutral"), Portrait(TEXT("Bruna_Neutral"))}, {TEXT("Angry"), Portrait(TEXT("Bruna_Angry"))}, {TEXT("Surprised"), Portrait(TEXT("Bruna_Surprised"))}};

	Milo->SpeakerId = TEXT("Milo");
	Milo->DisplayName = FText::FromString(TEXT("MILO"));
	Milo->AccentColor = FLinearColor(0.55f, 0.88f, 0.34f, 1.0f);
	Milo->PortraitSide = EChopItDialoguePortraitSide::Right;
	Milo->Portraits = {{TEXT("Neutral"), Portrait(TEXT("Milo_Neutral"))}, {TEXT("Angry"), Portrait(TEXT("Milo_Angry"))}, {TEXT("Surprised"), Portrait(TEXT("Milo_Surprised"))}};

	Death->SpeakerId = TEXT("Death");
	Death->DisplayName = FText::FromString(TEXT("DEATH"));
	Death->AccentColor = FLinearColor(0.78f, 0.80f, 0.84f, 1.0f);
	Death->PortraitSide = EChopItDialoguePortraitSide::Left;
	Death->Portraits = {{TEXT("Neutral"), Portrait(TEXT("Death_Neutral"))}};

	Oven->SpeakerId = TEXT("Oven");
	Oven->DisplayName = FText::FromString(TEXT("THE OVEN"));
	Oven->AccentColor = FLinearColor(1.0f, 0.20f, 0.015f, 1.0f);
	Oven->PortraitSide = EChopItDialoguePortraitSide::Right;
	Oven->NeutralExpression = TEXT("Hungry");
	Oven->Portraits = {{TEXT("Hungry"), Portrait(TEXT("Oven_Hungry"))}};

	Protagonist->SpeakerId = TEXT("Protagonist");
	Protagonist->DisplayName = FText::FromString(TEXT("YOU"));
	Protagonist->AccentColor = FLinearColor(1.0f, 0.56f, 0.12f, 1.0f);
	Protagonist->PortraitSide = EChopItDialoguePortraitSide::Left;
	Protagonist->NeutralExpression = TEXT("Surprised");
	Protagonist->Portraits = {{TEXT("Surprised"), Portrait(TEXT("Protagonist_Surprised"))}};

	Theme->PanelColor = FLinearColor(0.045f, 0.027f, 0.015f, 0.97f);
	Theme->BorderColor = FLinearColor(1.0f, 0.30f, 0.02f, 1.0f);
	Theme->TextColor = FLinearColor(0.96f, 0.86f, 0.66f, 1.0f);
	Theme->ChoiceColor = FLinearColor(0.16f, 0.085f, 0.035f, 0.98f);
	Theme->ChoiceSelectedColor = FLinearColor(0.86f, 0.20f, 0.015f, 1.0f);
	Theme->EnterDuration = 0.18f;
	Theme->ExitDuration = 0.22f;
	FChopItDialogueCameraAction& Closeup = Theme->CameraActions.FindOrAdd(TEXT("Closeup"));
	Closeup.Kind = EChopItDialogueCameraActionKind::Cue;
	Closeup.Cue = LoadObject<UChopItCameraCue>(nullptr, TEXT("/Game/ChopIt/Presentation/Camera/Cues/CC_DialogueCloseup.CC_DialogueCloseup"));
	Closeup.AnchorBinding = TEXT("CameraAnchor");
	Closeup.SubjectBinding = TEXT("Speaker");
	FChopItDialogueCameraAction& Impact = Theme->CameraActions.FindOrAdd(TEXT("Impact"));
	Impact.Kind = EChopItDialogueCameraActionKind::Shake;
	Impact.Shake = LoadObject<UCameraShakeAsset>(nullptr, TEXT("/Game/ChopIt/Presentation/Camera/Shakes/CS_Critical.CS_Critical"));
	Impact.SubjectBinding = TEXT("Speaker");
	Impact.Scale = 0.65f;
	auto ConfigureCloseup = [Theme](const FName ActionId, const FName AnchorBinding, const FName SubjectBinding, const float FieldOfView)
	{
		FChopItDialogueCameraAction& Action = Theme->CameraActions.FindOrAdd(ActionId);
		Action.Kind = EChopItDialogueCameraActionKind::Cue;
		Action.Cue = LoadObject<UChopItCameraCue>(nullptr, TEXT("/Game/ChopIt/Presentation/Camera/Cues/CC_DialogueCloseup.CC_DialogueCloseup"));
		Action.AnchorBinding = AnchorBinding;
		Action.SubjectBinding = SubjectBinding;
		Action.FieldOfViewOverride = FieldOfView;
		Action.BlendInTimeOverride = -1.0f;
	};
	ConfigureCloseup(TEXT("DeathCloseup"), TEXT("DeathCamera"), TEXT("Death"), 64.0f);
	ConfigureCloseup(TEXT("DeathZoomIn"), TEXT("DeathCamera"), TEXT("Death"), 58.0f);
	ConfigureCloseup(TEXT("DeathZoomOut"), TEXT("DeathCamera"), TEXT("Death"), 72.0f);
	ConfigureCloseup(TEXT("OvenCloseup"), TEXT("OvenCamera"), TEXT("Oven"), 55.0f);
	ConfigureCloseup(TEXT("OvenZoomIn"), TEXT("OvenCamera"), TEXT("Oven"), 44.0f);
	ConfigureCloseup(TEXT("OvenZoomOut"), TEXT("OvenCamera"), TEXT("Oven"), 66.0f);
	ConfigureCloseup(TEXT("OvenDevourZoom"), TEXT("OvenCamera"), TEXT("Oven"), 28.0f);
	Theme->CameraActions.FindChecked(TEXT("OvenDevourZoom")).BlendInTimeOverride = 0.055f;
	ConfigureCloseup(TEXT("PlayerCloseup"), TEXT("PlayerCamera"), TEXT("Player"), 57.0f);
	ConfigureCloseup(TEXT("PlayerZoomIn"), TEXT("PlayerCamera"), TEXT("Player"), 49.0f);
	ConfigureCloseup(TEXT("PlayerZoomOut"), TEXT("PlayerCamera"), TEXT("Player"), 66.0f);
	ConfigureCloseup(TEXT("IntroWide"), TEXT("WideCamera"), TEXT("Oven"), 72.0f);
	FChopItDialogueCameraAction& OvenImpact = Theme->CameraActions.FindOrAdd(TEXT("OvenImpact"));
	OvenImpact.Kind = EChopItDialogueCameraActionKind::Shake;
	OvenImpact.Shake = LoadObject<UCameraShakeAsset>(nullptr, TEXT("/Game/ChopIt/Presentation/Camera/Shakes/CS_Critical.CS_Critical"));
	OvenImpact.SubjectBinding = TEXT("Oven");
	OvenImpact.Scale = 0.9f;
	OvenImpact.bSustainUntilLineEnds = true;

	const FGameplayTag ChoiceTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Dialogue.Event.Choice")), false);
	const FGameplayTag CompleteTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Dialogue.Event.DemoComplete")), false);
	Sequence->DialogueId = TEXT("DialogueDemo");
	Sequence->Theme = Theme;
	Sequence->EntryLineId = TEXT("BrunaStart");
	Sequence->bPauseWorld = true;
	Sequence->bBlockGameplayInput = true;
	Sequence->bCanCancel = true;
	Sequence->Lines.Reset();

	FChopItDialogueLine& BrunaStart = Sequence->Lines.AddDefaulted_GetRef();
	BrunaStart.LineId = TEXT("BrunaStart");
	BrunaStart.Speaker = Bruna;
	BrunaStart.Expression = TEXT("Neutral");
	BrunaStart.Text = FText::FromString(TEXT("Milo... did you hear that?\n\nThe <shake amp=\"2.5\"><wave amp=\"5\"><color value=\"#FF5A13\"><pulse scale=\"0.12\">forest spoke</pulse></color></wave></shake>.<pause seconds=\"0.25\"/><cue id=\"Warning\" event=\"Dialogue.Event.Warning\" face=\"Angry\" camera=\"Impact\" target=\"Speaker\"/> And it did not sound happy."));
	BrunaStart.NextLineId = TEXT("MiloChoice");
	BrunaStart.CameraAction = TEXT("Closeup");

	FChopItDialogueLine& MiloChoice = Sequence->Lines.AddDefaulted_GetRef();
	MiloChoice.LineId = TEXT("MiloChoice");
	MiloChoice.Speaker = Milo;
	MiloChoice.Expression = TEXT("Surprised");
	MiloChoice.Text = FText::FromString(TEXT("Perfect. My first day and I am already arguing with trees.\n\nWhat do we do, boss?"));
	FChopItDialogueChoice& FaceIt = MiloChoice.Choices.AddDefaulted_GetRef();
	FaceIt.ChoiceId = TEXT("FaceIt");
	FaceIt.Text = FText::FromString(TEXT("Stand our ground and hear what it wants."));
	FaceIt.NextLineId = TEXT("BrunaFinal");
	FaceIt.EventTag = ChoiceTag;
	FChopItDialogueChoice& Leave = MiloChoice.Choices.AddDefaulted_GetRef();
	Leave.ChoiceId = TEXT("Leave");
	Leave.Text = FText::FromString(TEXT("Run until the forest forgets us."));
	Leave.NextLineId = TEXT("MiloFinal");
	Leave.EventTag = ChoiceTag;

	FChopItDialogueLine& BrunaFinal = Sequence->Lines.AddDefaulted_GetRef();
	BrunaFinal.LineId = TEXT("BrunaFinal");
	BrunaFinal.Speaker = Bruna;
	BrunaFinal.Expression = TEXT("Angry");
	BrunaFinal.Text = FText::FromString(TEXT("That is what I wanted to hear. Grab the axe and keep your eyes open.\n\nIf it speaks again, this time <color value=\"#FFB22E\"><size value=\"1.18\">we answer back</size></color>."));
	BrunaFinal.NextLineId = TEXT("Outro");

	FChopItDialogueLine& MiloFinal = Sequence->Lines.AddDefaulted_GetRef();
	MiloFinal.LineId = TEXT("MiloFinal");
	MiloFinal.Speaker = Milo;
	MiloFinal.Expression = TEXT("Angry");
	MiloFinal.Text = FText::FromString(TEXT("Great plan. Just one problem: <shake amp=\"1.5\">I think it already knows where we live</shake>."));
	MiloFinal.NextLineId = TEXT("Outro");

	FChopItDialogueLine& Outro = Sequence->Lines.AddDefaulted_GetRef();
	Outro.LineId = TEXT("Outro");
	Outro.Text = FText::FromString(TEXT("<speed value=\"1.5\"><color value=\"#FF6A00\">The night has only just begun.</color></speed>"));
	Outro.EndEvent = CompleteTag;

	const FGameplayTag IntroStartedTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Dialogue.Event.IntroStarted")), false);
	const FGameplayTag DeathGestureTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Dialogue.Event.DeathGesture")), false);
	const FGameplayTag ChainRevealTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Dialogue.Event.ChainReveal")), false);
	const FGameplayTag ChainPullTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Dialogue.Event.ChainPull")), false);
	const FGameplayTag OvenRoarTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Dialogue.Event.OvenRoar")), false);
	const FGameplayTag DeathVanishTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Dialogue.Event.DeathVanish")), false);
	const FGameplayTag QuestStartTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Dialogue.Event.QuestStart")), false);
	const FGameplayTag IntroCompleteTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Dialogue.Event.IntroComplete")), false);
	MatchIntro->DialogueId = TEXT("MatchIntro");
	MatchIntro->Theme = Theme;
	MatchIntro->EntryLineId = TEXT("DeathArrival");
	MatchIntro->bPauseWorld = true;
	MatchIntro->bBlockGameplayInput = true;
	MatchIntro->bCanCancel = false;
	MatchIntro->StartEvent = IntroStartedTag;
	MatchIntro->EndEvent = IntroCompleteTag;
	MatchIntro->Lines.Reset();

	FChopItDialogueLine& DeathArrival = MatchIntro->Lines.AddDefaulted_GetRef();
	DeathArrival.LineId = TEXT("DeathArrival");
	DeathArrival.Speaker = Death;
	DeathArrival.Expression = TEXT("Neutral");
	DeathArrival.Text = FText::FromString(TEXT("<cue id=\"Arrival\" event=\"Dialogue.Event.DeathGesture\" target=\"Death\"/>Wake up.<pause seconds=\"0.30\"/> You have already wasted <color value=\"#FFC766\"><speed value=\"0.86\">enough daylight</speed></color>."));
	DeathArrival.NextLineId = TEXT("PlayerWakes");
	DeathArrival.CameraAction = TEXT("DeathZoomIn");

	FChopItDialogueLine& PlayerWakes = MatchIntro->Lines.AddDefaulted_GetRef();
	PlayerWakes.LineId = TEXT("PlayerWakes");
	PlayerWakes.Speaker = Protagonist;
	PlayerWakes.Expression = TEXT("Surprised");
	PlayerWakes.Text = FText::FromString(TEXT("Where am I...? And why do I have an axe?\n\n<cue id=\"ChainReveal\" event=\"Dialogue.Event.ChainReveal\" target=\"Player\"/><shake amp=\"1.4\">Wait...</shake> what the hell am I <color value=\"#FF8A36\"><pulse scale=\"0.08\">chained to</pulse></color>?"));
	PlayerWakes.NextLineId = TEXT("DeathPact");
	PlayerWakes.CameraAction = TEXT("PlayerZoomOut");

	FChopItDialogueLine& DeathPact = MatchIntro->Lines.AddDefaulted_GetRef();
	DeathPact.LineId = TEXT("DeathPact");
	DeathPact.Speaker = Death;
	DeathPact.Text = FText::FromString(TEXT("To the <color value=\"#FF4A16\"><pulse scale=\"0.12\"><size value=\"1.16\">Oven</size></pulse></color>.<pause seconds=\"0.25\"/> I forged this pact and bound the chain to your soul.\n\nRun, strike its links, or beg all you want: <color value=\"#E5E7EB\"><size value=\"1.08\">you cannot break it or escape its reach</size></color>."));
	DeathPact.NextLineId = TEXT("OvenWakes");
	DeathPact.CameraAction = TEXT("IntroWide");

	FChopItDialogueLine& OvenWakes = MatchIntro->Lines.AddDefaulted_GetRef();
	OvenWakes.LineId = TEXT("OvenWakes");
	OvenWakes.Speaker = Oven;
	OvenWakes.Expression = TEXT("Hungry");
	OvenWakes.Text = FText::FromString(TEXT("<cue id=\"Ignite\" event=\"Dialogue.Event.OvenRoar\" camera=\"OvenImpact\" target=\"Oven\"/><shake amp=\"3.1\"><pulse scale=\"0.16\"><size value=\"1.38\"><color value=\"#FF3B0A\">HUNGER.</color></size></pulse></shake>"));
	OvenWakes.NextLineId = TEXT("PlayerJoke");
	OvenWakes.CameraAction = TEXT("OvenZoomIn");

	FChopItDialogueLine& PlayerJoke = MatchIntro->Lines.AddDefaulted_GetRef();
	PlayerJoke.LineId = TEXT("PlayerJoke");
	PlayerJoke.Speaker = Protagonist;
	PlayerJoke.Expression = TEXT("Surprised");
	PlayerJoke.Text = FText::FromString(TEXT("Perfect. I am trapped, carrying an axe, and <wave amp=\"2.4\"><color value=\"#FF8A36\">the chimney wants to eat me</color></wave>.\n\nA perfectly normal morning."));
	PlayerJoke.NextLineId = TEXT("DeathFuel");
	PlayerJoke.CameraAction = TEXT("PlayerZoomIn");

	FChopItDialogueLine& DeathFuel = MatchIntro->Lines.AddDefaulted_GetRef();
	DeathFuel.LineId = TEXT("DeathFuel");
	DeathFuel.Speaker = Death;
	DeathFuel.Text = FText::FromString(TEXT("The Oven <color value=\"#FF6A00\"><pulse scale=\"0.06\">burns without rest</pulse></color>. If it receives no fuel, it will find <shake amp=\"0.8\"><color value=\"#FF4A16\">something else to burn</color></shake>."));
	DeathFuel.NextLineId = TEXT("DeathQuota");
	DeathFuel.CameraAction = TEXT("DeathZoomOut");

	FChopItDialogueLine& DeathQuota = MatchIntro->Lines.AddDefaulted_GetRef();
	DeathQuota.LineId = TEXT("DeathQuota");
	DeathQuota.Speaker = Death;
	DeathQuota.Text = FText::FromString(TEXT("Before <color value=\"#C9A9FF\">night falls</color>, you will chop and deliver <pulse scale=\"0.12\"><color value=\"#FFB22E\"><size value=\"1.24\">{Quota} units of wood</size></color></pulse>."));
	DeathQuota.NextLineId = TEXT("PlayerAsksFailure");
	DeathQuota.CameraAction = TEXT("IntroWide");

	FChopItDialogueLine& PlayerAsksFailure = MatchIntro->Lines.AddDefaulted_GetRef();
	PlayerAsksFailure.LineId = TEXT("PlayerAsksFailure");
	PlayerAsksFailure.Speaker = Protagonist;
	PlayerAsksFailure.Expression = TEXT("Surprised");
	PlayerAsksFailure.Text = FText::FromString(TEXT("And what happens if I do not collect all <color value=\"#FFB22E\"><size value=\"1.12\">{Quota}</size></color>?\n\nDoes it get angry? Does it puff smoke in disappointment?"));
	PlayerAsksFailure.NextLineId = TEXT("OvenThreat");
	PlayerAsksFailure.CameraAction = TEXT("PlayerZoomIn");

	FChopItDialogueLine& OvenThreat = MatchIntro->Lines.AddDefaulted_GetRef();
	OvenThreat.LineId = TEXT("OvenThreat");
	OvenThreat.Speaker = Oven;
	OvenThreat.Expression = TEXT("Hungry");
	OvenThreat.Text = FText::FromString(TEXT("<cue id=\"ChainPull\" event=\"Dialogue.Event.ChainPull\" target=\"Player\"/><shake amp=\"2.2\"><color value=\"#FF6A00\">The chain will bring you.</color></shake>\n\n<pause seconds=\"0.30\"/><cue id=\"DevourZoom\" camera=\"OvenDevourZoom\" target=\"Oven\"/><cue id=\"DevourShake\" camera=\"OvenImpact\" target=\"Oven\"/><speed value=\"0.74\"><shake amp=\"3.4\"><wave amp=\"4.2\"><pulse scale=\"0.14\"><size value=\"1.34\"><color value=\"#FF2400\">I WILL DEVOUR YOU.</color></size></pulse></wave></shake></speed>"));
	OvenThreat.NextLineId = TEXT("DeathClarifies");
	OvenThreat.CameraAction = TEXT("OvenZoomOut");

	FChopItDialogueLine& DeathClarifies = MatchIntro->Lines.AddDefaulted_GetRef();
	DeathClarifies.LineId = TEXT("DeathClarifies");
	DeathClarifies.Speaker = Death;
	DeathClarifies.Text = FText::FromString(TEXT("At the end of the day, I will count the wood. If <color value=\"#FF4A16\"><size value=\"1.10\">even one unit is missing</size></color>, the Oven will drag you into its mouth.\n\nWood is the only thing that can satisfy it.<pause seconds=\"0.30\"/> <speed value=\"0.78\"><color value=\"#C9A9FF\">For now.</color></speed>"));
	DeathClarifies.CameraAction = TEXT("DeathZoomOut");
	FChopItDialogueChoice& Accept = DeathClarifies.Choices.AddDefaulted_GetRef();
	Accept.ChoiceId = TEXT("AcceptQuota");
	Accept.Text = FText::FromString(TEXT("ACCEPT — Feed the Oven."));
	Accept.NextLineId = TEXT("PlayerAccepts");
	Accept.EventTag = ChoiceTag;
	FChopItDialogueChoice& Defy = DeathClarifies.Choices.AddDefaulted_GetRef();
	Defy.ChoiceId = TEXT("DefyOven");
	Defy.Text = FText::FromString(TEXT("DEFY — Use the axe against it."));
	Defy.NextLineId = TEXT("PlayerDefies");
	Defy.EventTag = ChoiceTag;
	FChopItDialogueChoice& Ask = DeathClarifies.Choices.AddDefaulted_GetRef();
	Ask.ChoiceId = TEXT("AskWhy");
	Ask.Text = FText::FromString(TEXT("ASK — Demand an explanation."));
	Ask.NextLineId = TEXT("PlayerAsksWhy");
	Ask.EventTag = ChoiceTag;

	FChopItDialogueLine& PlayerAccepts = MatchIntro->Lines.AddDefaulted_GetRef();
	PlayerAccepts.LineId = TEXT("PlayerAccepts");
	PlayerAccepts.Speaker = Protagonist;
	PlayerAccepts.Expression = TEXT("Surprised");
	PlayerAccepts.Text = FText::FromString(TEXT("So I chop <color value=\"#FFB22E\"><size value=\"1.14\">{Quota}</size></color>, feed the monster, and keep breathing.<pause seconds=\"0.20\"/> I can work with that."));
	PlayerAccepts.NextLineId = TEXT("DeathAccepts");
	PlayerAccepts.CameraAction = TEXT("PlayerCloseup");

	FChopItDialogueLine& DeathAccepts = MatchIntro->Lines.AddDefaulted_GetRef();
	DeathAccepts.LineId = TEXT("DeathAccepts");
	DeathAccepts.Speaker = Death;
	DeathAccepts.Text = FText::FromString(TEXT("You learn quickly.<pause seconds=\"0.22\"/> Perhaps you will live to see <color value=\"#FFC766\">another dawn</color>."));
	DeathAccepts.NextLineId = TEXT("DeathFinal");
	DeathAccepts.CameraAction = TEXT("DeathZoomIn");

	FChopItDialogueLine& PlayerDefies = MatchIntro->Lines.AddDefaulted_GetRef();
	PlayerDefies.LineId = TEXT("PlayerDefies");
	PlayerDefies.Speaker = Protagonist;
	PlayerDefies.Expression = TEXT("Surprised");
	PlayerDefies.Text = FText::FromString(TEXT("I have an axe. What stops me from using it <shake amp=\"1.0\"><color value=\"#FF8A36\">against the Oven</color></shake>?"));
	PlayerDefies.NextLineId = TEXT("DeathDefiance");
	PlayerDefies.CameraAction = TEXT("PlayerCloseup");

	FChopItDialogueLine& DeathDefiance = MatchIntro->Lines.AddDefaulted_GetRef();
	DeathDefiance.LineId = TEXT("DeathDefiance");
	DeathDefiance.Speaker = Death;
	DeathDefiance.Text = FText::FromString(TEXT("The axe can wound trees. <color value=\"#C9A9FF\"><size value=\"1.10\">Not the pact.</size></color>\n\nBut go ahead... the Oven enjoys it when its food approaches willingly."));
	DeathDefiance.NextLineId = TEXT("DeathFinal");
	DeathDefiance.CameraAction = TEXT("DeathZoomIn");

	FChopItDialogueLine& PlayerAsksWhy = MatchIntro->Lines.AddDefaulted_GetRef();
	PlayerAsksWhy.LineId = TEXT("PlayerAsksWhy");
	PlayerAsksWhy.Speaker = Protagonist;
	PlayerAsksWhy.Expression = TEXT("Surprised");
	PlayerAsksWhy.Text = FText::FromString(TEXT("Why me? What did I do to deserve <color value=\"#C9A9FF\">this pact</color>?"));
	PlayerAsksWhy.NextLineId = TEXT("DeathRefuses");
	PlayerAsksWhy.CameraAction = TEXT("PlayerCloseup");

	FChopItDialogueLine& DeathRefuses = MatchIntro->Lines.AddDefaulted_GetRef();
	DeathRefuses.LineId = TEXT("DeathRefuses");
	DeathRefuses.Speaker = Death;
	DeathRefuses.Text = FText::FromString(TEXT("That will not feed the Oven or stop nightfall.\n\nSurvive the day and <speed value=\"0.82\"><color value=\"#C9A9FF\">perhaps tomorrow you will earn an answer</color></speed>."));
	DeathRefuses.NextLineId = TEXT("DeathFinal");
	DeathRefuses.CameraAction = TEXT("DeathZoomIn");

	FChopItDialogueLine& DeathFinal = MatchIntro->Lines.AddDefaulted_GetRef();
	DeathFinal.LineId = TEXT("DeathFinal");
	DeathFinal.Speaker = Death;
	DeathFinal.Text = FText::FromString(TEXT("<cue id=\"FinalGesture\" event=\"Dialogue.Event.DeathGesture\" target=\"Death\"/>Before nightfall: <pulse scale=\"0.10\"><color value=\"#FFB22E\"><size value=\"1.20\">{Quota} units of wood</size></color></pulse>. <color value=\"#FF4A16\"><size value=\"1.12\">Not one less.</size></color>\n\nNow take the axe and start chopping.<cue id=\"QuestStart\" event=\"Dialogue.Event.QuestStart\" target=\"Oven\"/><cue id=\"Vanish\" event=\"Dialogue.Event.DeathVanish\" target=\"Death\"/>"));
	DeathFinal.CameraAction = TEXT("DeathZoomIn");
	DeathFinal.EndEvent = IntroCompleteTag;

	if (!ChopItBootstrap::SaveAsset(Bruna) || !ChopItBootstrap::SaveAsset(Milo)
		|| !ChopItBootstrap::SaveAsset(Death) || !ChopItBootstrap::SaveAsset(Oven) || !ChopItBootstrap::SaveAsset(Protagonist)
		|| !ChopItBootstrap::SaveAsset(Theme) || !ChopItBootstrap::SaveAsset(Sequence) || !ChopItBootstrap::SaveAsset(MatchIntro)) return false;
	return PlaceDeathInStartupMap() && PlaceDeliveryZonesInStartupMap() && PlaceChainObstaclesInStartupMap()
		&& RebuildPhase1Map(ChopItBootstrap::DialogueMap, true);
}

bool UChopItBootstrapCommandlet::PlaceDeathInStartupMap() const
{
	if (!FPackageName::DoesPackageExist(ChopItBootstrap::StartupMap))
	{
		UE_LOG(LogChopIt, Error, TEXT("Cannot place Death; startup map does not exist."));
		return false;
	}

	UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(ChopItBootstrap::StartupMap);
	if (!IsValid(World))
	{
		UE_LOG(LogChopIt, Error, TEXT("Could not load L_Startup to place Death."));
		return false;
	}

	AChopItQuotaMachine* Oven = nullptr;
	for (TActorIterator<AChopItQuotaMachine> It(World); It; ++It)
	{
		Oven = *It;
		break;
	}
	// L_Startup currently creates its quota machine at runtime. Use the same
	// deterministic fallback transform as AChopItPlayerController when no authored
	// machine exists yet, so the placed Death still appears beside the spawned oven.
	const FVector OvenLocation = Oven ? Oven->GetActorLocation() : FVector(180.0f, -480.0f, 0.0f);

	const FName DeathTag(TEXT("Dialogue.Death"));
	AChopItDialogueStageCharacter* DeathActor = nullptr;
	for (TActorIterator<AChopItDialogueStageCharacter> It(World); It; ++It)
	{
		if (It->ActorHasTag(DeathTag) || It->GetFName() == TEXT("Death_NPC"))
		{
			DeathActor = *It;
			break;
		}
	}

	const FVector DeathLocation = OvenLocation + FVector(-70.0f, 320.0f, 0.0f);
	if (!DeathActor)
	{
		FActorSpawnParameters Parameters;
		Parameters.Name = TEXT("Death_NPC");
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		DeathActor = World->SpawnActor<AChopItDialogueStageCharacter>(DeathLocation, FRotator::ZeroRotator, Parameters);
	}
	if (!DeathActor) return false;

	DeathActor->Tags.AddUnique(DeathTag);
	DeathActor->SetActorLabel(TEXT("DEATH — INTRO NPC"));
	DeathActor->SetActorLocationAndRotation(DeathLocation, FRotator::ZeroRotator);
	DeathActor->ConfigurePillMarker(FText::FromString(TEXT("DEATH")));

	const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, ChopItBootstrap::StartupMap);
	UE_LOG(LogChopIt, Display, TEXT("Death NPC placed in L_Startup: %s"), bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}

bool UChopItBootstrapCommandlet::PlaceDeliveryZonesInStartupMap() const
{
	if (!FPackageName::DoesPackageExist(ChopItBootstrap::StartupMap))
	{
		UE_LOG(LogChopIt, Error, TEXT("Cannot place delivery zones; startup map does not exist."));
		return false;
	}

	UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(ChopItBootstrap::StartupMap);
	if (!IsValid(World))
	{
		UE_LOG(LogChopIt, Error, TEXT("Could not load L_Startup to place delivery zones."));
		return false;
	}

	AChopItQuotaMachine* Oven = nullptr;
	for (TActorIterator<AChopItQuotaMachine> It(World); It; ++It)
	{
		Oven = *It;
		break;
	}
	const FVector OvenLocation = Oven ? Oven->GetActorLocation() : FVector(180.0f, -480.0f, 0.0f);

	AChopItDeliveryZone* Delivery = nullptr;
	for (TActorIterator<AChopItDeliveryZone> It(World); It; ++It)
	{
		Delivery = *It;
		break;
	}
	const FVector DeliveryLocation = OvenLocation + FVector(340.0f, 220.0f, 15.0f);
	if (!Delivery)
	{
		FActorSpawnParameters Parameters;
		Parameters.Name = TEXT("StartupQuotaDeliveryZone");
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Delivery = World->SpawnActor<AChopItDeliveryZone>(DeliveryLocation, FRotator::ZeroRotator, Parameters);
	}
	if (!Delivery) return false;
	Delivery->SetActorLocationAndRotation(DeliveryLocation, FRotator::ZeroRotator);
	Delivery->SetActorLabel(TEXT("WOOD DELIVERY — E"));

	AChopItWoodGrantZone* GrantZone = nullptr;
	for (TActorIterator<AChopItWoodGrantZone> It(World); It; ++It)
	{
		GrantZone = *It;
		break;
	}
	const FVector GrantLocation = OvenLocation + FVector(720.0f, 860.0f, 15.0f);
	if (!GrantZone)
	{
		FActorSpawnParameters Parameters;
		Parameters.Name = TEXT("StartupWoodGrantZone_200");
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GrantZone = World->SpawnActor<AChopItWoodGrantZone>(GrantLocation, FRotator::ZeroRotator, Parameters);
	}
	if (!GrantZone) return false;
	GrantZone->SetActorLocationAndRotation(GrantLocation, FRotator::ZeroRotator);
	GrantZone->SetActorLabel(TEXT("TEST — E: GET 200 LOGS"));

	const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, ChopItBootstrap::StartupMap);
	UE_LOG(LogChopIt, Display, TEXT("Startup delivery and 200-log test zones placed: %s"),
		bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}

bool UChopItBootstrapCommandlet::PlaceChainObstaclesInStartupMap() const
{
	if (!FPackageName::DoesPackageExist(ChopItBootstrap::StartupMap))
	{
		UE_LOG(LogChopIt, Error, TEXT("Cannot place chain obstacles; startup map does not exist."));
		return false;
	}

	UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(ChopItBootstrap::StartupMap);
	if (!IsValid(World))
	{
		UE_LOG(LogChopIt, Error, TEXT("Could not load L_Startup to place chain obstacles."));
		return false;
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* Wood = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Wood"));
	UMaterialInterface* Stone = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Stone"));
	UMaterialInterface* Roof = ChopItBootstrap::LoadBlockoutMaterial(TEXT("MI_Roof"));
	if (!Cube || !Cylinder || !Sphere || !Wood || !Stone || !Roof)
	{
		UE_LOG(LogChopIt, Error, TEXT("Missing meshes or materials required by the Startup chain obstacle course."));
		return false;
	}

	// Remove the previously generated course first. This keeps the operation
	// deterministic and allows its layout to be tuned without accumulating actors.
	TArray<AActor*> ExistingObstacles;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->GetName().StartsWith(TEXT("StartupChainObstacle_")))
		{
			ExistingObstacles.Add(*It);
		}
	}
	for (AActor* Existing : ExistingObstacles)
	{
		// DestroyActor keeps the UObject alive until garbage collection. Retire its
		// object name first so the deterministic replacement can be spawned during
		// this same commandlet pass without LevelActor's duplicate-name fatal error.
		const FName RetiredName = MakeUniqueObjectName(
			World->PersistentLevel, Existing->GetClass(), TEXT("RetiredStartupChainObstacle"));
		Existing->Rename(
			*RetiredName.ToString(), World->PersistentLevel,
			REN_DontCreateRedirectors | REN_NonTransactional);
		World->DestroyActor(Existing);
	}

	AChopItQuotaMachine* Oven = nullptr;
	for (TActorIterator<AChopItQuotaMachine> It(World); It; ++It)
	{
		Oven = *It;
		break;
	}
	const FVector OvenLocation = Oven ? Oven->GetActorLocation() : FVector(180.0f, -480.0f, 0.0f);
	int32 SpawnedCount = 0;

	// A broad semicircle on the open side of the house. The delivery pad and the
	// player's initial spawn remain clear, while circling these posts creates stable
	// multi-anchor wraps just like the first section of L_Test_ChainLab.
	const FVector PostOffsets[] =
	{
		FVector(280.0f, -300.0f, 165.0f),
		FVector(570.0f, -360.0f, 165.0f),
		FVector(830.0f, -220.0f, 165.0f),
		FVector(900.0f, 80.0f, 165.0f),
		FVector(850.0f, 390.0f, 165.0f)
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(PostOffsets); ++Index)
	{
		const FName PostName(*FString::Printf(TEXT("StartupChainObstacle_WrapPost_%02d"), Index));
		AStaticMeshActor* Post = ChopItBootstrap::SpawnBlockoutMesh(
			World, Cylinder, PostName, OvenLocation + PostOffsets[Index], FVector(0.92f, 0.92f, 3.3f), Wood);
		if (!Post) return false;
		Post->Tags.AddUnique(TEXT("ChopIt.ChainObstacle"));
		++SpawnedCount;

		const FName CapName(*FString::Printf(TEXT("StartupChainObstacle_PostCap_%02d"), Index));
		AStaticMeshActor* Cap = ChopItBootstrap::SpawnBlockoutMesh(
			World, Sphere, CapName, OvenLocation + PostOffsets[Index] + FVector(0.0f, 0.0f, 170.0f),
			FVector(1.08f, 1.08f, 0.38f), Roof, TEXT("NoCollision"));
		if (!Cap) return false;
		++SpawnedCount;
	}

	// Two perpendicular stone walls provide hard corners so the authoritative tether
	// path can retain and release corner anchors, not only slide around cylinders.
	AStaticMeshActor* CornerA = ChopItBootstrap::SpawnBlockoutMesh(
		World, Cube, TEXT("StartupChainObstacle_Corner_A"), OvenLocation + FVector(610.0f, -690.0f, 115.0f),
		FVector(0.42f, 2.8f, 2.3f), Stone);
	AStaticMeshActor* CornerB = ChopItBootstrap::SpawnBlockoutMesh(
		World, Cube, TEXT("StartupChainObstacle_Corner_B"), OvenLocation + FVector(810.0f, -530.0f, 115.0f),
		FVector(2.4f, 0.42f, 2.3f), Stone);
	if (!CornerA || !CornerB) return false;
	CornerA->Tags.AddUnique(TEXT("ChopIt.ChainObstacle"));
	CornerB->Tags.AddUnique(TEXT("ChopIt.ChainObstacle"));
	SpawnedCount += 2;

	FStaticMeshCompilingManager::Get().FinishAllCompilation();
	const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, ChopItBootstrap::StartupMap);
	UE_LOG(LogChopIt, Display, TEXT("Startup chain obstacle course placed: actors=%d, saved=%s"),
		SpawnedCount, bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
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
		TEXT("/Game/ChopIt/AI/Enemies/DA_Enemy_Tree"), TEXT("DA_Enemy_Tree"), TEXT("Tree"), TEXT("Animated Tree"), 48.0f, 250.0f, 7.0f, 1.0f, 6);
	UChopItEnemyDefinition* Fast = CreateEnemy(
		TEXT("/Game/ChopIt/AI/Enemies/DA_Enemy_FastTree"), TEXT("DA_Enemy_FastTree"), TEXT("FastTree"), TEXT("Swift Tree"), 32.0f, 410.0f, 5.0f, 0.75f, 10);
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
		{ TEXT("Filo"), TEXT("Sharpened Edge"), TEXT("+20% axe damage"), EChopItUpgradeRarity::Common, 5, 1.0f,
			{ Modifier(EChopItCombatStat::Damage, EChopItModifierOperation::Multiply, 1.20f) } },
		{ TEXT("Ritmo"), TEXT("Tireless Arms"), TEXT("+15% attack speed"), EChopItUpgradeRarity::Common, 5, 1.0f,
			{ Modifier(EChopItCombatStat::AttackSpeed, EChopItModifierOperation::Multiply, 1.15f) } },
		{ TEXT("Alcance"), TEXT("Extending Handle"), TEXT("+18% range"), EChopItUpgradeRarity::Common, 5, 1.0f,
			{ Modifier(EChopItCombatStat::Range, EChopItModifierOperation::Multiply, 1.18f) } },
		{ TEXT("Critico"), TEXT("Woodcutter's Eye"), TEXT("+8% critical chance"), EChopItUpgradeRarity::Common, 5, 0.9f,
			{ Modifier(EChopItCombatStat::CriticalChance, EChopItModifierOperation::Add, 0.08f) } },
		{ TEXT("Botas"), TEXT("Oiled Boots"), TEXT("+12% movement speed"), EChopItUpgradeRarity::Common, 5, 0.9f,
			{ Modifier(EChopItCombatStat::MovementSpeed, EChopItModifierOperation::Multiply, 1.12f) } },
		{ TEXT("Furia"), TEXT("Clumsy Fury"), TEXT("+35% damage, -10% attack speed"), EChopItUpgradeRarity::Rare, 3, 0.35f,
			{ Modifier(EChopItCombatStat::Damage, EChopItModifierOperation::Multiply, 1.35f), Modifier(EChopItCombatStat::AttackSpeed, EChopItModifierOperation::Multiply, 0.90f) } },
		{ TEXT("Precision"), TEXT("Forest Compass"), TEXT("+12% critical chance and +10% range"), EChopItUpgradeRarity::Uncommon, 3, 0.6f,
			{ Modifier(EChopItCombatStat::CriticalChance, EChopItModifierOperation::Add, 0.12f), Modifier(EChopItCombatStat::Range, EChopItModifierOperation::Multiply, 1.10f) } },
		{ TEXT("Gigante"), TEXT("Giant Axe"), TEXT("+25% damage and range, -10% attack speed"), EChopItUpgradeRarity::Rare, 3, 0.35f,
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
	Day->DayDuration = 5.0f;
	Day->DuskMinimumDuration = 2.0f;
	Day->DuskHardDeadline = 2.0f;
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
	UInputAction* CameraLookAction = ChopItBootstrap::LoadOrCreateAsset<UInputAction>(ChopItBootstrap::CameraLookActionPackage, TEXT("IA_CameraLook"));
	UInputAction* CameraZoomAction = ChopItBootstrap::LoadOrCreateAsset<UInputAction>(ChopItBootstrap::CameraZoomActionPackage, TEXT("IA_CameraZoom"));
	UInputAction* CameraResetAction = ChopItBootstrap::LoadOrCreateAsset<UInputAction>(ChopItBootstrap::CameraResetActionPackage, TEXT("IA_CameraReset"));
	UInputMappingContext* GameplayContext = ChopItBootstrap::LoadOrCreateAsset<UInputMappingContext>(
		ChopItBootstrap::GameplayContextPackage, TEXT("IMC_Gameplay"));
	UInputAction* DialogueAdvance = ChopItBootstrap::LoadOrCreateAsset<UInputAction>(ChopItBootstrap::DialogueAdvanceActionPackage, TEXT("IA_DialogueAdvance"));
	UInputAction* DialogueNext = ChopItBootstrap::LoadOrCreateAsset<UInputAction>(ChopItBootstrap::DialogueNextChoiceActionPackage, TEXT("IA_DialogueNextChoice"));
	UInputAction* DialoguePrevious = ChopItBootstrap::LoadOrCreateAsset<UInputAction>(ChopItBootstrap::DialoguePreviousChoiceActionPackage, TEXT("IA_DialoguePreviousChoice"));
	UInputAction* DialogueCancel = ChopItBootstrap::LoadOrCreateAsset<UInputAction>(ChopItBootstrap::DialogueCancelActionPackage, TEXT("IA_DialogueCancel"));
	UInputMappingContext* DialogueContext = ChopItBootstrap::LoadOrCreateAsset<UInputMappingContext>(ChopItBootstrap::DialogueContextPackage, TEXT("IMC_Dialogue"));
	if (!MoveAction || !InteractAction || !CameraLookAction || !CameraZoomAction || !CameraResetAction || !GameplayContext
		|| !DialogueAdvance || !DialogueNext || !DialoguePrevious || !DialogueCancel || !DialogueContext)
	{
		return false;
	}

	MoveAction->ValueType = EInputActionValueType::Axis2D;
	InteractAction->ValueType = EInputActionValueType::Boolean;
	CameraLookAction->ValueType = EInputActionValueType::Axis2D;
	CameraZoomAction->ValueType = EInputActionValueType::Axis1D;
	CameraResetAction->ValueType = EInputActionValueType::Boolean;
	for (UInputAction* DialogueAction : {DialogueAdvance, DialogueNext, DialoguePrevious, DialogueCancel})
	{
		DialogueAction->ValueType = EInputActionValueType::Boolean;
		DialogueAction->bTriggerWhenPaused = true;
		DialogueAction->bConsumeInput = true;
	}
	GameplayContext->UnmapAll();
	DialogueContext->UnmapAll();

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
	GameplayContext->MapKey(CameraLookAction, EKeys::Mouse2D);
	GameplayContext->MapKey(CameraLookAction, EKeys::Gamepad_Right2D);
	GameplayContext->MapKey(CameraZoomAction, EKeys::MouseWheelAxis);
	GameplayContext->MapKey(CameraZoomAction, EKeys::Gamepad_DPad_Up);
	FEnhancedActionKeyMapping& ZoomOut = GameplayContext->MapKey(CameraZoomAction, EKeys::Gamepad_DPad_Down);
	AddNegate(ZoomOut, true, false);
	GameplayContext->MapKey(CameraResetAction, EKeys::MiddleMouseButton);
	GameplayContext->MapKey(CameraResetAction, EKeys::Gamepad_RightThumbstick);

	DialogueContext->MapKey(DialogueAdvance, EKeys::E);
	DialogueContext->MapKey(DialogueAdvance, EKeys::Enter);
	DialogueContext->MapKey(DialogueAdvance, EKeys::SpaceBar);
	DialogueContext->MapKey(DialogueAdvance, EKeys::Gamepad_FaceButton_Bottom);
	DialogueContext->MapKey(DialogueNext, EKeys::S);
	DialogueContext->MapKey(DialogueNext, EKeys::Down);
	DialogueContext->MapKey(DialogueNext, EKeys::Gamepad_DPad_Down);
	DialogueContext->MapKey(DialoguePrevious, EKeys::W);
	DialogueContext->MapKey(DialoguePrevious, EKeys::Up);
	DialogueContext->MapKey(DialoguePrevious, EKeys::Gamepad_DPad_Up);
	DialogueContext->MapKey(DialogueCancel, EKeys::Escape);
	DialogueContext->MapKey(DialogueCancel, EKeys::Gamepad_FaceButton_Right);

	const bool bSaved = ChopItBootstrap::SaveAsset(MoveAction)
		&& ChopItBootstrap::SaveAsset(InteractAction)
		&& ChopItBootstrap::SaveAsset(CameraLookAction)
		&& ChopItBootstrap::SaveAsset(CameraZoomAction)
		&& ChopItBootstrap::SaveAsset(CameraResetAction)
		&& ChopItBootstrap::SaveAsset(GameplayContext)
		&& ChopItBootstrap::SaveAsset(DialogueAdvance)
		&& ChopItBootstrap::SaveAsset(DialogueNext)
		&& ChopItBootstrap::SaveAsset(DialoguePrevious)
		&& ChopItBootstrap::SaveAsset(DialogueCancel)
		&& ChopItBootstrap::SaveAsset(DialogueContext);
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

bool UChopItBootstrapCommandlet::CreateCameraAssets() const
{
	auto CreateRig = [](const TCHAR* PackagePath, const TCHAR* AssetName, const float FOV) -> UCameraRigAsset*
	{
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), PackagePath, AssetName);
		if (UCameraRigAsset* Existing = LoadObject<UCameraRigAsset>(nullptr, *ObjectPath)) return Existing;
		UPackage* Package = CreatePackage(PackagePath);
		UE::Cameras::FCameraRigAssetAssembler Assembler(FName(AssetName), Package);
		UCameraRigAsset* Rig = Assembler.Get();
		Rig->SetFlags(RF_Public | RF_Standalone);
		Assembler.MakeArrayRootNode().AddArrayChild<UFieldOfViewCameraNode>()
			.SetParameter(&UFieldOfViewCameraNode::FieldOfView, FOV).Done().Done().BuildCameraRig();
		FAssetRegistryModule::AssetCreated(Rig);
		return Rig;
	};

	UCameraRigAsset* GameplayRig = CreateRig(TEXT("/Game/ChopIt/Presentation/Camera/Rigs/CR_GameplayOrbit"), TEXT("CR_GameplayOrbit"), 85.0f);
	UCameraRigAsset* ScriptedRig = CreateRig(TEXT("/Game/ChopIt/Presentation/Camera/Rigs/CR_DialogueAnchor"), TEXT("CR_DialogueAnchor"), 65.0f);
	UCameraRigAsset* CinematicRig = CreateRig(TEXT("/Game/ChopIt/Presentation/Camera/Rigs/CR_Cinematic"), TEXT("CR_Cinematic"), 60.0f);
	UCameraRigAsset* DeathRig = CreateRig(TEXT("/Game/ChopIt/Presentation/Camera/Rigs/CR_Death"), TEXT("CR_Death"), 55.0f);
	if (!GameplayRig || !ScriptedRig || !CinematicRig || !DeathRig) return false;
	UMaterial* OcclusionMaterial = ChopItBootstrap::LoadOrCreateAsset<UMaterial>(TEXT("/Game/ChopIt/Presentation/Camera/Materials/M_CameraFoliageOcclusion"), TEXT("M_CameraFoliageOcclusion"));
	if (OcclusionMaterial)
	{
		// Rebuild this asset deterministically. A masked temporal dither keeps depth sorting
		// stable and works for any opaque mesh without requiring bespoke translucent variants.
		const TArray<TObjectPtr<UMaterialExpression>> ExistingExpressions = OcclusionMaterial->GetExpressionCollection().Expressions;
		for (UMaterialExpression* Expression : ExistingExpressions)
		{
			// MaterialEditingLibrary roots expressions while editing. Its deletion path
			// marks them as garbage, which asserts for rooted objects in UE 5.8.
			if (Expression && Expression->IsRooted()) Expression->RemoveFromRoot();
			UMaterialEditingLibrary::DeleteMaterialExpression(OcclusionMaterial, Expression);
		}
		OcclusionMaterial->BlendMode = BLEND_Masked;
		OcclusionMaterial->SetShadingModel(MSM_Unlit);
		OcclusionMaterial->TwoSided = true;
		OcclusionMaterial->OpacityMaskClipValue = 0.3333f;
		UMaterialExpressionConstant3Vector* Color = CastChecked<UMaterialExpressionConstant3Vector>(UMaterialEditingLibrary::CreateMaterialExpression(OcclusionMaterial, UMaterialExpressionConstant3Vector::StaticClass(), -240, 0));
		Color->Constant = FLinearColor(0.08f, 0.09f, 0.1f);
		UMaterialEditingLibrary::ConnectMaterialProperty(Color, TEXT(""), MP_EmissiveColor);

		UMaterialExpressionScalarParameter* Visibility = CastChecked<UMaterialExpressionScalarParameter>(UMaterialEditingLibrary::CreateMaterialExpression(OcclusionMaterial, UMaterialExpressionScalarParameter::StaticClass(), -420, 180));
		Visibility->ParameterName = TEXT("Visibility");
		Visibility->DefaultValue = 0.28f;
		UMaterialFunctionInterface* DitherFunction = LoadObject<UMaterialFunctionInterface>(nullptr, TEXT("/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA.DitherTemporalAA"));
		if (DitherFunction)
		{
			UMaterialExpressionMaterialFunctionCall* Dither = CastChecked<UMaterialExpressionMaterialFunctionCall>(UMaterialEditingLibrary::CreateMaterialExpression(OcclusionMaterial, UMaterialExpressionMaterialFunctionCall::StaticClass(), -160, 180));
			Dither->SetMaterialFunction(DitherFunction);
			int32 AlphaInputIndex = INDEX_NONE;
			for (int32 InputIndex = 0; InputIndex < Dither->FunctionInputs.Num(); ++InputIndex)
			{
				if (Dither->GetInputName(InputIndex).ToString().Contains(TEXT("Alpha")))
				{
					AlphaInputIndex = InputIndex;
					break;
				}
			}
			if (AlphaInputIndex == INDEX_NONE && !Dither->FunctionInputs.IsEmpty()) AlphaInputIndex = 0;
			if (AlphaInputIndex != INDEX_NONE) Dither->FunctionInputs[AlphaInputIndex].Input.Connect(0, Visibility);
			UMaterialEditingLibrary::ConnectMaterialProperty(Dither, TEXT(""), MP_OpacityMask);
		}
		else
		{
			UE_LOG(LogChopIt, Error, TEXT("Could not load the engine DitherTemporalAA material function"));
		}
		UMaterialEditingLibrary::RecompileMaterial(OcclusionMaterial);
	}
	if (UArrayCameraNode* GameplayRoot = Cast<UArrayCameraNode>(GameplayRig->RootNode))
	{
		// Occlusion is evaluated by UChopItCameraComponent after the final manual orbit
		// transform. The plugin node evaluates an earlier rig pose and therefore cannot
		// reliably see what is actually between the rendered camera and its subject.
		for (int32 NodeIndex = GameplayRoot->Children.Num() - 1; NodeIndex >= 0; --NodeIndex)
		{
			if (GameplayRoot->Children[NodeIndex] && GameplayRoot->Children[NodeIndex]->IsA<UOcclusionMaterialCameraNode>()) GameplayRoot->Children.RemoveAt(NodeIndex);
		}
		GameplayRig->BuildCameraRig();
	}

	UStateTree* StateTree = ChopItBootstrap::LoadOrCreateAsset<UStateTree>(ChopItBootstrap::CameraStateTreePackage, TEXT("ST_CameraDirector"));
	const FName EditorDataName = MakeUniqueObjectName(StateTree, UStateTreeEditorData::StaticClass(), TEXT("EditorData"));
	UStateTreeEditorData* EditorData = NewObject<UStateTreeEditorData>(StateTree, EditorDataName, RF_Transactional);
	if (!EditorData) return false;
	StateTree->EditorData = EditorData;
	EditorData->Schema = NewObject<UCameraDirectorStateTreeSchema>(EditorData);
	if (!EditorData->Schema) return false;
	UStateTreeState& Root = EditorData->AddSubTree(TEXT("CameraRoot"));
	struct FCameraStateDefinition
	{
		const TCHAR* Name;
		EChopItCameraMode Mode;
		UCameraRigAsset* Rig;
	};
	const FCameraStateDefinition Definitions[] =
	{
		{ TEXT("GameplayOrbit"), EChopItCameraMode::GameplayOrbit, GameplayRig },
		{ TEXT("Scripted"), EChopItCameraMode::Scripted, ScriptedRig },
		{ TEXT("Cinematic"), EChopItCameraMode::Cinematic, CinematicRig },
		{ TEXT("Death"), EChopItCameraMode::Death, DeathRig }
	};
	TArray<UStateTreeState*> CameraStates;
	for (const FCameraStateDefinition& Definition : Definitions)
	{
		UStateTreeState& State = Root.AddChildState(FName(Definition.Name));
		State.AddEnterCondition<FChopItCameraModeCondition>().GetInstanceData().ExpectedMode = Definition.Mode;
		State.AddTask<FChopItActivateCameraRigTask>().GetInstanceData().CameraRig = Definition.Rig;
		CameraStates.Add(&State);
	}
	for (int32 FromIndex = 0; FromIndex < CameraStates.Num(); ++FromIndex)
	{
		for (int32 ToIndex = 0; ToIndex < CameraStates.Num(); ++ToIndex)
		{
			if (FromIndex == ToIndex) continue;
			FStateTreeTransition& Transition = CameraStates[FromIndex]->AddTransition(
				EStateTreeTransitionTrigger::OnTick,
				EStateTreeTransitionType::GotoState,
				CameraStates[ToIndex]);
			Transition.AddConditionWithOuter<FChopItCameraModeCondition>(CameraStates[FromIndex]).GetInstanceData().ExpectedMode = Definitions[ToIndex].Mode;
		}
	}
	FStateTreeCompilerLog CompilerLog;
	FStateTreeCompiler Compiler(CompilerLog);
	if (!Compiler.Compile(*StateTree))
	{
		CompilerLog.DumpToLog(StateTree, LogChopIt);
		UE_LOG(LogChopIt, Error, TEXT("Camera StateTree failed to compile."));
		return false;
	}

	UCameraAsset* CameraAsset = ChopItBootstrap::LoadOrCreateAsset<UCameraAsset>(ChopItBootstrap::CameraAssetPackage, TEXT("CA_PlayerCameras"));
	if (!CameraAsset) return false;
	USingleCameraDirector* Director = Cast<USingleCameraDirector>(CameraAsset->GetCameraDirector());
	if (!Director)
	{
		// The component owns mode switching and final transforms. Keeping the
		// Gameplay Cameras host on one valid base rig avoids making runtime camera
		// startup depend on the experimental StateTree director ABI in UE 5.8.
		const FName DirectorName = MakeUniqueObjectName(
			CameraAsset,
			USingleCameraDirector::StaticClass(),
			TEXT("GameplayDirector"));
		Director = NewObject<USingleCameraDirector>(CameraAsset, DirectorName, RF_Transactional);
	}
	if (!Director) return false;
	Director->CameraRig = GameplayRig;
	CameraAsset->SetCameraDirector(Director);
	CameraAsset->BuildCamera();

	auto CreateShake = [](const TCHAR* PackagePath, const TCHAR* AssetName, const float Amplitude, const float Frequency, const float Duration) -> UCameraShakeAsset*
	{
		UCameraShakeAsset* Asset = ChopItBootstrap::LoadOrCreateAsset<UCameraShakeAsset>(PackagePath, FName(AssetName));
		if (!Asset) return nullptr;
		UEnvelopeShakeCameraNode* Envelope = FindObject<UEnvelopeShakeCameraNode>(Asset, TEXT("Envelope"));
		if (!Envelope) Envelope = NewObject<UEnvelopeShakeCameraNode>(Asset, TEXT("Envelope"));
		UPerlinNoiseLocationShakeCameraNode* Noise = FindObject<UPerlinNoiseLocationShakeCameraNode>(Asset, TEXT("Noise"));
		if (!Noise) Noise = NewObject<UPerlinNoiseLocationShakeCameraNode>(Asset, TEXT("Noise"));
		if (!Envelope || !Noise) return nullptr;
		Noise->X = {Amplitude, Frequency}; Noise->Y = {Amplitude, Frequency}; Noise->Z = {Amplitude * 0.35f, Frequency};
		Noise->Octaves.Value = 2;
		Envelope->EaseInTime.Value = 0.02f; Envelope->EaseOutTime.Value = FMath::Min(0.12f, Duration * 0.5f); Envelope->TotalTime.Value = Duration; Envelope->Shake = Noise;
		Asset->RootNode = Envelope;
		Asset->BuildCameraShake();
		return Asset;
	};
	UCameraShakeAsset* NormalShake = CreateShake(TEXT("/Game/ChopIt/Presentation/Camera/Shakes/CS_Normal"), TEXT("CS_Normal"), 4.0f, 24.0f, 0.16f);
	UCameraShakeAsset* CriticalShake = CreateShake(TEXT("/Game/ChopIt/Presentation/Camera/Shakes/CS_Critical"), TEXT("CS_Critical"), 9.0f, 20.0f, 0.28f);
	UCameraShakeAsset* HeavyShake = CreateShake(TEXT("/Game/ChopIt/Presentation/Camera/Shakes/CS_HeavyImpact"), TEXT("CS_HeavyImpact"), 15.0f, 15.0f, 0.42f);

	auto CreateEffect = [](const TCHAR* RigPackage, const TCHAR* RigName, const TCHAR* PresetPackage, const TCHAR* PresetName, const float Vignette, const float Saturation, const float Duration, const int32 Order)
	{
		const FString RigObjectPath = FString::Printf(TEXT("%s.%s"), RigPackage, RigName);
		UCameraRigAsset* Rig = LoadObject<UCameraRigAsset>(nullptr, *RigObjectPath);
		if (!Rig)
		{
			UPackage* Package = CreatePackage(RigPackage);
			UE::Cameras::FCameraRigAssetAssembler Assembler(FName(RigName), Package);
			Rig = Assembler.Get(); Rig->SetFlags(RF_Public | RF_Standalone);
			Assembler.MakeArrayRootNode().AddArrayChild<UPostProcessCameraNode>().Setup([Vignette, Saturation](UPostProcessCameraNode* Node)
			{
				Node->PostProcessSettings.bOverride_VignetteIntensity = true;
				Node->PostProcessSettings.VignetteIntensity = Vignette;
				Node->PostProcessSettings.bOverride_ColorSaturation = true;
				Node->PostProcessSettings.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.0f);
			}).Done().Done().BuildCameraRig();
			FAssetRegistryModule::AssetCreated(Rig);
		}
		UChopItCameraEffectPreset* Preset = ChopItBootstrap::LoadOrCreateAsset<UChopItCameraEffectPreset>(PresetPackage, FName(PresetName));
		Preset->ModifierRig = Rig; Preset->DefaultDuration = Duration; Preset->OrderKey = Order;
		return ChopItBootstrap::SaveAsset(Rig) && ChopItBootstrap::SaveAsset(Preset);
	};
	const bool bEffects =
		CreateEffect(TEXT("/Game/ChopIt/Presentation/Camera/Effects/CRV_DialogueFocus"), TEXT("CRV_DialogueFocus"), TEXT("/Game/ChopIt/Presentation/Camera/Effects/CE_DialogueFocus"), TEXT("CE_DialogueFocus"), 0.55f, 0.85f, 0.0f, 200)
		&& CreateEffect(TEXT("/Game/ChopIt/Presentation/Camera/Effects/CRV_DamageFlash"), TEXT("CRV_DamageFlash"), TEXT("/Game/ChopIt/Presentation/Camera/Effects/CE_DamageFlash"), TEXT("CE_DamageFlash"), 0.8f, 0.45f, 0.18f, 300)
		&& CreateEffect(TEXT("/Game/ChopIt/Presentation/Camera/Effects/CRV_Danger"), TEXT("CRV_Danger"), TEXT("/Game/ChopIt/Presentation/Camera/Effects/CE_Danger"), TEXT("CE_Danger"), 0.72f, 0.65f, 0.0f, 100);

	UChopItCameraCue* DialogueCue = ChopItBootstrap::LoadOrCreateAsset<UChopItCameraCue>(TEXT("/Game/ChopIt/Presentation/Camera/Cues/CC_DialogueCloseup"), TEXT("CC_DialogueCloseup"));
	DialogueCue->Mode = EChopItCameraMode::Scripted; DialogueCue->CameraRig = ScriptedRig; DialogueCue->Priority = 300; DialogueCue->FieldOfView = 55.0f;
	DialogueCue->InputLocks = static_cast<int32>(EChopItCameraInputLock::Camera | EChopItCameraInputLock::Movement | EChopItCameraInputLock::Actions);

	auto SaveGenerated = [](UObject* Asset)
	{
		return ChopItBootstrap::SaveAsset(Asset) || (Asset && FPackageName::DoesPackageExist(Asset->GetOutermost()->GetName()));
	};
	const bool bSaved = bEffects && SaveGenerated(OcclusionMaterial)
		&& SaveGenerated(GameplayRig) && SaveGenerated(ScriptedRig)
		&& SaveGenerated(CinematicRig) && SaveGenerated(DeathRig)
		&& SaveGenerated(StateTree) && SaveGenerated(CameraAsset)
		&& SaveGenerated(NormalShake) && SaveGenerated(CriticalShake)
		&& SaveGenerated(HeavyShake) && SaveGenerated(DialogueCue);
	UE_LOG(LogChopIt, Display, TEXT("Gameplay Cameras assets: %s"), bSaved ? TEXT("OK") : TEXT("FAILED"));
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

bool UChopItBootstrapCommandlet::CreateDamageTextMaterial() const
{
	constexpr TCHAR PackageName[] = TEXT("/Game/ChopIt/Presentation/Materials/M_DamageText_AlwaysOnTop");
	UMaterial* DamageTextMaterial = LoadObject<UMaterial>(
		nullptr,
		TEXT("/Game/ChopIt/Presentation/Materials/M_DamageText_AlwaysOnTop.M_DamageText_AlwaysOnTop"));
	if (!DamageTextMaterial)
	{
		UMaterial* SourceMaterial = LoadObject<UMaterial>(
			nullptr,
			TEXT("/Engine/EngineMaterials/DefaultTextMaterialTranslucent.DefaultTextMaterialTranslucent"));
		if (!SourceMaterial)
		{
			return false;
		}
		UPackage* Package = CreatePackage(PackageName);
		DamageTextMaterial = DuplicateObject<UMaterial>(SourceMaterial, Package, TEXT("M_DamageText_AlwaysOnTop"));
		if (!DamageTextMaterial)
		{
			return false;
		}
		FAssetRegistryModule::AssetCreated(DamageTextMaterial);
	}

	DamageTextMaterial->bDisableDepthTest = true;
	DamageTextMaterial->PostEditChange();
	const bool bSaved = ChopItBootstrap::SaveAsset(DamageTextMaterial);
	UE_LOG(LogChopIt, Display, TEXT("Always-on-top damage text material: %s"), bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}

bool UChopItBootstrapCommandlet::RebuildPhase1Map(
	const FString& LongPackageName,
	const bool bFullSandbox,
	const bool bIncludeCombatDummies,
	const bool bHarvestTestLayout,
	const bool bEconomyTestLayout) const
{
	FStaticMeshCompilingManager::Get().FinishAllCompilation();
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

	AStaticMeshActor* GroundActor = ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("Ground"), FVector(0, 0, -50), FVector(42, 42, 1), Ground);
	if (!GroundActor) return false;
	ChopItBootstrap::MarkCameraSolid(GroundActor);
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
		if (BoundaryActor)
		{
			ChopItBootstrap::MarkCameraSolid(BoundaryActor);
			BoundaryActor->SetActorHiddenInGame(true);
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = TEXT("PlayerStart");
	APlayerStart* PlayerStart = World->SpawnActor<APlayerStart>(FVector(750, 0, 100), FRotator(0, 0, 0), SpawnParameters);
	if (!PlayerStart) return false;
	PlayerStart->SetActorLabel(TEXT("PlayerStart"));
	if (LongPackageName == ChopItBootstrap::SandboxMap)
	{
		SpawnParameters.Name = TEXT("CameraSystemDemo");
		AChopItCameraDemoTrigger* Demo = World->SpawnActor<AChopItCameraDemoTrigger>(FVector(930, 180, 60), FRotator::ZeroRotator, SpawnParameters);
		if (!Demo) return false;
		Demo->SetActorLabel(TEXT("Camera Demo: interact to play shot/effect/shake/restore"));
	}
	else if (LongPackageName == ChopItBootstrap::DialogueMap)
	{
		SpawnParameters.Name = TEXT("DialogueDemoTrigger");
		AChopItDialogueTrigger* DialogueTrigger = World->SpawnActor<AChopItDialogueTrigger>(FVector(930, 180, 60), FRotator::ZeroRotator, SpawnParameters);
		SpawnParameters.Name = TEXT("DialogueCameraAnchor");
		const FVector AnchorLocation(560.0f, 520.0f, 240.0f);
		const FVector SubjectLocation(930.0f, 180.0f, 140.0f);
		AChopItCameraAnchor* DialogueAnchor = World->SpawnActor<AChopItCameraAnchor>(AnchorLocation, (SubjectLocation - AnchorLocation).Rotation(), SpawnParameters);
		UChopItDialogueSequence* DemoSequence = LoadObject<UChopItDialogueSequence>(nullptr, TEXT("/Game/ChopIt/Dialogue/Sequences/DA_Dialogue_Demo.DA_Dialogue_Demo"));
		if (!DialogueTrigger || !DialogueAnchor || !DemoSequence) return false;
		DialogueTrigger->SetActorLabel(TEXT("Dialogue Demo: interact to test text, portraits, choices and camera"));
		DialogueTrigger->Sequence = DemoSequence;
		DialogueTrigger->CameraAnchor = DialogueAnchor;
		DialogueAnchor->DefaultSubject = DialogueTrigger;
		DialogueAnchor->SetActorLabel(TEXT("Dialogue Camera Anchor"));
	}

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
	if (!DirectionalLight || !DirectionalLight->GetLightComponent()) return false;
	DirectionalLight->SetActorLabel(TEXT("DirectionalLight"));
	DirectionalLight->GetLightComponent()->SetIntensity(5.0f);
	DirectionalLight->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.88f, 0.70f));
	DirectionalLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);

	SpawnParameters.Name = TEXT("SkyLight");
	ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	if (!SkyLight || !SkyLight->GetLightComponent()) return false;
	SkyLight->SetActorLabel(TEXT("SkyLight"));
	SkyLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	SkyLight->GetLightComponent()->SetIntensity(1.2f);
	SkyLight->GetLightComponent()->SetRealTimeCapture(true);

	SpawnParameters.Name = TEXT("SkyAtmosphere");
	ASkyAtmosphere* SkyAtmosphere = World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	if (!SkyAtmosphere) return false;
	SkyAtmosphere->SetActorLabel(TEXT("SkyAtmosphere"));

	SpawnParameters.Name = TEXT("HeightFog");
	AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(FVector(0, 0, -50), FRotator::ZeroRotator, SpawnParameters);
	if (!Fog) return false;
	Fog->SetActorLabel(TEXT("HeightFog"));

	SpawnParameters.Name = TEXT("NavMeshBounds");
	ANavMeshBoundsVolume* NavBounds = World->SpawnActor<ANavMeshBoundsVolume>(FVector(0, 0, 200), FRotator::ZeroRotator, SpawnParameters);
	if (!NavBounds) return false;
	NavBounds->SetActorLabel(TEXT("NavMeshBounds"));
	NavBounds->SetActorScale3D(FVector(22, 22, 5));
	FStaticMeshCompilingManager::Get().FinishAllCompilation();
	FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::EditorMode);
	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavigationSystem) return false;
	NavigationSystem->RemoveNavigationBuildLock(
		ENavigationBuildLock::AsyncLoadLock,
		UNavigationSystemV1::ELockRemovalRebuildAction::NoRebuild);
	FNavigationSystem::Build(*World);

	const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, LongPackageName);
	UE_LOG(LogChopIt, Display, TEXT("Rebuilt Phase 1 map %s: %s"), *LongPackageName, bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}

bool UChopItBootstrapCommandlet::RebuildNavigationData(const FString& LongPackageName) const
{
	if (!FPackageName::DoesPackageExist(LongPackageName))
	{
		UE_LOG(LogChopIt, Error, TEXT("Cannot rebuild navigation; map does not exist: %s"), *LongPackageName);
		return false;
	}

	UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(LongPackageName);
	if (!IsValid(World))
	{
		UE_LOG(LogChopIt, Error, TEXT("Could not load map for navigation rebuild: %s"), *LongPackageName);
		return false;
	}

	FStaticMeshCompilingManager::Get().FinishAllCompilation();
	FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::EditorMode);
	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavigationSystem)
	{
		UE_LOG(LogChopIt, Error, TEXT("Navigation system was not created for %s"), *LongPackageName);
		return false;
	}
	NavigationSystem->RemoveNavigationBuildLock(
		ENavigationBuildLock::AsyncLoadLock,
		UNavigationSystemV1::ELockRemovalRebuildAction::NoRebuild);
	FNavigationSystem::Build(*World);
	if (!NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
	{
		UE_LOG(LogChopIt, Error, TEXT("Navigation build produced no navigation data for %s"), *LongPackageName);
		return false;
	}
	const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, LongPackageName);
	UE_LOG(LogChopIt, Display, TEXT("Rebuilt navigation for %s: %s"), *LongPackageName, bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}

bool UChopItBootstrapCommandlet::RebuildChainLabMap() const
{
	FStaticMeshCompilingManager::Get().FinishAllCompilation();
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
	AStaticMeshActor* GroundActor = ChopItBootstrap::SpawnBlockoutMesh(World, Cube, TEXT("ChainLab_Ground"), FVector(0, 0, -50), FVector(50, 50, 1), Ground);
	if (!GroundActor) return false;
	ChopItBootstrap::MarkCameraSolid(GroundActor);
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
		if (!ChainLabBlueprint->GeneratedClass->IsChildOf(AChopItQuotaMachine::StaticClass()))
		{
			UE_LOG(LogChopIt, Error, TEXT("BP_ChainLabMachine generated an incompatible class: %s"), *ChainLabBlueprint->GeneratedClass->GetPathName());
			return false;
		}
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
			ChopItBootstrap::MarkCameraSolid(Actor);
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
	FStaticMeshCompilingManager::Get().FinishAllCompilation();
	const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, ChopItBootstrap::ChainLabMap);
	UE_LOG(LogChopIt, Display, TEXT("Rebuilt Chain Lab %s: %s"), ChopItBootstrap::ChainLabMap, bSaved ? TEXT("OK") : TEXT("FAILED"));
	return bSaved;
}
