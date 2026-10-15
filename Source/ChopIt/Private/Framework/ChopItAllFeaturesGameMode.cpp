#include "Framework/ChopItAllFeaturesGameMode.h"
#include "Terrain/ChopItTerrainActor.h"
#include "Terrain/ChopItTerrainProjectSettings.h"
#include "Terrain/ChopItTerrainMeshBuilder.h"
#include "Harvest/ChopItTree.h"
#include "Economy/ChopItCabinHub.h"
#include "Economy/ChopItQuotaMachine.h"
#include "Economy/ChopItDeliveryZone.h"
#include "Economy/ChopItSellZone.h"
#include "Economy/ChopItWoodGrantZone.h"
#include "Shop/ChopItShopTerminal.h"
#include "Targets/ChopItCombatDummy.h"
#include "Dialogue/ChopItDialogueTrigger.h"
#include "Camera/ChopItCameraDemoTrigger.h"
#include "Camera/ChopItCameraAnchor.h"
#include "Events/ChopItSpecialEventPlacement.h"
#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneSpawnConfig.h"
#include "Phone/ChopItPhoneVariantComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/Guid.h"

AChopItAllFeaturesGameMode::AChopItAllFeaturesGameMode()
{
	PhoneSpawnConfig = TSoftObjectPtr<UChopItPhoneSpawnConfig>(
		FSoftObjectPath(TEXT("/Game/NewChopIt/Art/Models/Phone/DA_PhoneSpawn.DA_PhoneSpawn")));
}

void AChopItAllFeaturesGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	if (!bGenerateTerrainAtRuntime)
	{
		SpawnMysteryPhones();
		return;
	}
	UChopItTerrainDataAsset* Recipe = GetDefault<UChopItTerrainProjectSettings>()->DefaultTerrain.LoadSynchronous();
	if (!Recipe)
	{
		ErrorMessage = TEXT("Select a default terrain Data Asset in Project Settings > Game > ChopIt Terrain.");
		UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMessage);
		return;
	}
	static TOptional<int32> PreviousSeed;
	TerrainRunSeed = static_cast<int32>(FGuid::NewGuid().A);
	if (PreviousSeed.IsSet() && TerrainRunSeed == PreviousSeed.GetValue())
		TerrainRunSeed = TerrainRunSeed == MAX_int32 ? MIN_int32 : TerrainRunSeed + 1;
	PreviousSeed = TerrainRunSeed;
	FChopItTerrainSettings RunSettings = Recipe->MakeSettings(TerrainRunSeed);
	for (TActorIterator<APlayerStart> Start(GetWorld()); Start; ++Start)
	{
		const FVector Position = Start->GetActorLocation();
		RunSettings.SpawnFlatCenter = FVector2D(Position.X, Position.Y);
		break;
	}
	const FIntPoint MeshCells = FChopItTerrainMeshBuilder::GetCellCounts(
		FVector2D(RunSettings.Width, RunSettings.Length), Recipe->Mesh.PolygonDetail);
	const double MeshStepX = RunSettings.Width / MeshCells.X;
	const double MeshStepY = RunSettings.Length / MeshCells.Y;
	const float MeshPadding = float(FMath::Sqrt(MeshStepX * MeshStepX + MeshStepY * MeshStepY));
	// Pad for a whole mesh cell so its triangles cannot slope into the spawn disc.
	RunSettings.SpawnFlatRadius = FMath::Max(RunSettings.SpawnFlatRadius, 1000.0f) + MeshPadding;
	RunSettings.SpawnFlatBlendWidth = FMath::Max(RunSettings.SpawnFlatBlendWidth, 500.0f);
	// A map can keep an editor preview of the terrain. Rebuild that actor for each
	// run so the preview material is preserved and Play never has two terrains.
	for (TActorIterator<AChopItTerrainActor> TerrainIt(GetWorld()); TerrainIt; ++TerrainIt)
	{
		GeneratedTerrain = *TerrainIt;
		break;
	}
	if (GeneratedTerrain)
	{
		GeneratedTerrain->TerrainSettings = RunSettings;
		GeneratedTerrain->MeshSettings = Recipe->Mesh;
		if (!GeneratedTerrain->RebuildTerrain(ErrorMessage)) GeneratedTerrain = nullptr;
	}
	else
	{
		GeneratedTerrain = Recipe->SpawnTerrainWithSettings(this, FTransform::Identity, RunSettings, ErrorMessage);
	}
	if (!GeneratedTerrain)
	{
		UE_LOG(LogTemp, Error, TEXT("Full feature terrain generation failed: %s"), *ErrorMessage);
		return;
	}

	int32 AlignedActors = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor == GeneratedTerrain) continue;
		const bool bGroundedFeature = Actor->IsA<AChopItTree>() || Actor->IsA<AChopItCabinHub>()
			|| Actor->IsA<AChopItQuotaMachine>() || Actor->IsA<AChopItDeliveryZone>()
			|| Actor->IsA<AChopItSellZone>() || Actor->IsA<AChopItShopTerminal>()
			|| Actor->IsA<AChopItCombatDummy>() || Actor->IsA<AChopItDialogueTrigger>()
			|| Actor->IsA<AChopItCameraDemoTrigger>() || Actor->IsA<AChopItCameraAnchor>()
			|| Actor->IsA<AChopItWoodGrantZone>() || Actor->IsA<APlayerStart>()
			|| Actor->ActorHasTag(TEXT("AlignToGeneratedTerrain"));
		if (!bGroundedFeature) continue;
		FVector Position = Actor->GetActorLocation();
		float GroundZ = 0.0f;
		if (!GeneratedTerrain->SampleWorldHeight(Position, GroundZ)) continue;
		Position.Z += GroundZ;
		Actor->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
		++AlignedActors;
	}
	UE_LOG(LogTemp, Display, TEXT("Full feature world generated: asset=%s seed=%d alignedActors=%d flatCenter=%s flatRadius=%.0f samples=%dx%d"),
		*Recipe->GetPathName(), TerrainRunSeed, AlignedActors,
		*RunSettings.SpawnFlatCenter.ToString(), RunSettings.SpawnFlatRadius,
		GeneratedTerrain->TerrainSettings.SamplesX, GeneratedTerrain->TerrainSettings.SamplesY);
	SpawnMysteryPhones();
}

void AChopItAllFeaturesGameMode::SpawnMysteryPhones()
{
	SpawnedPhones.Reset();
	if (!GetWorld()) return;
	for (TActorIterator<AChopItMysteryPhone> It(GetWorld()); It; ++It)
		SpawnedPhones.Add(*It);
	if (!SpawnedPhones.IsEmpty())
	{
		UE_LOG(LogTemp, Display, TEXT("Phone spawn: using %d phone(s) placed in the level."), SpawnedPhones.Num());
		return;
	}
	if (!GeneratedTerrain) return;
	const UChopItPhoneSpawnConfig* Config = PhoneSpawnConfig.LoadSynchronous();
	if (!Config || !Config->IsValidConfig())
	{
		UE_LOG(LogTemp, Warning, TEXT("Phone spawn skipped: missing or invalid configuration %s."),
			*PhoneSpawnConfig.ToString());
		return;
	}
	APlayerStart* Start = nullptr;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It) { Start = *It; break; }
	if (!Start)
	{
		UE_LOG(LogTemp, Warning, TEXT("Phone spawn skipped: no PlayerStart."));
		return;
	}
	// Separate random stream: adding terrain or reward draws cannot move this event.
	FRandomStream Random(TerrainRunSeed ^ int32(0x50484f4e));
	if (Random.FRand() >= Config->SpawnProbability) return;
	UClass* PhoneClass = Config->PhoneClass ? Config->PhoneClass.Get() : AChopItMysteryPhone::StaticClass();
	TArray<FVector> UsedLocations;
	for (int32 Index = 0; Index < Config->MaxPhones; ++Index)
	{
		FVector Ground;
		bool bFound = FChopItSpecialEventPlacement::FindLocation(*GetWorld(), *GeneratedTerrain,
			Start->GetActorLocation(), Config->Placement, Random, UsedLocations, Ground);
		if (!bFound && Index == 0)
		{
			// The broad search can miss the small playable clearing on terrain
			// recipes with a high automatic water level. Search that area directly.
			FChopItSpecialEventPlacementRules ClearingRules = Config->Placement;
			const float ClearingRadius = GeneratedTerrain->TerrainSettings.SpawnFlatRadius;
			ClearingRules.MinDistanceFromStart = FMath::Min(ClearingRules.MinDistanceFromStart, 600.0f);
			ClearingRules.MaxDistanceFromStart = FMath::Min(1800.0f,
				ClearingRadius - ClearingRules.FootprintRadius * 2.0f);
			ClearingRules.RouteSampleStep = 0.0f;
			ClearingRules.MaxAttempts = FMath::Max(ClearingRules.MaxAttempts, 256);
			if (ClearingRules.MaxDistanceFromStart > ClearingRules.MinDistanceFromStart)
			{
				bFound = FChopItSpecialEventPlacement::FindLocation(*GetWorld(), *GeneratedTerrain,
					Start->GetActorLocation(), ClearingRules, Random, UsedLocations, Ground);
				if (bFound)
					UE_LOG(LogTemp, Display, TEXT("Phone spawn: using spawn clearing fallback."));
			}
		}
		if (!bFound)
		{
			UE_LOG(LogTemp, Warning, TEXT("Phone spawn stopped after %d/%d: no valid location in the regular search or spawn clearing."),
				Index, Config->MaxPhones);
			break;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AChopItMysteryPhone* Phone = GetWorld()->SpawnActor<AChopItMysteryPhone>(PhoneClass,
			Ground + FVector::UpVector * Config->GroundOffset, FRotator::ZeroRotator, Params);
		if (!Phone)
		{
			UE_LOG(LogTemp, Warning, TEXT("Phone spawn failed at %s."), *Ground.ToCompactString());
			continue;
		}
		if (UChopItPhoneVariantComponent* Variant = Phone->GetPhoneVariant())
		{
			const uint32 VariantSeed = static_cast<uint32>(TerrainRunSeed)
				^ (static_cast<uint32>(Index + 1) * 0x9E3779B9u) ^ 0x56415249u;
			Variant->InitializeSelectionSeed(static_cast<int32>(VariantSeed));
		}
		UsedLocations.Add(Ground);
		SpawnedPhones.Add(Phone);
		UE_LOG(LogTemp, Display, TEXT("Phone spawned: %s at %s."),
			*Phone->GetName(), *Phone->GetActorLocation().ToCompactString());
	}
	UE_LOG(LogTemp, Display, TEXT("Phone spawn: seed=%d count=%d."), TerrainRunSeed, SpawnedPhones.Num());
}
