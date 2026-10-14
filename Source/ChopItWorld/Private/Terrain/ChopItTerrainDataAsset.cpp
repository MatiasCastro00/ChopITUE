#include "Terrain/ChopItTerrainDataAsset.h"
#include "Terrain/ChopItTerrainActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"

namespace
{
void TransferMatchingProperties(const UScriptStruct* FromStruct, const void* From, const UScriptStruct* ToStruct, void* To)
{
	for (TFieldIterator<FProperty> It(FChopItTerrainProfile::StaticStruct()); It; ++It)
	{
		FProperty* ProfileProperty = *It;
		FProperty* SettingsProperty = ToStruct == FChopItTerrainSettings::StaticStruct()
			? ToStruct->FindPropertyByName(ProfileProperty->GetFName()) : FromStruct->FindPropertyByName(ProfileProperty->GetFName());
		checkf(SettingsProperty && SettingsProperty->SameType(ProfileProperty), TEXT("Terrain profile and runtime settings diverged: %s"), *ProfileProperty->GetName());
		const FProperty* SourceProperty = FromStruct == FChopItTerrainSettings::StaticStruct() ? SettingsProperty : ProfileProperty;
		FProperty* DestProperty = ToStruct == FChopItTerrainSettings::StaticStruct() ? SettingsProperty : ProfileProperty;
		DestProperty->CopyCompleteValue(DestProperty->ContainerPtrToValuePtr<void>(To),
			SourceProperty->ContainerPtrToValuePtr<void>(From));
	}
}
}

FChopItTerrainProfile::FChopItTerrainProfile()
{
	for (TFieldIterator<FProperty> It(FChopItTerrainSettings::StaticStruct()); It; ++It)
		if (It->GetFName() != GET_MEMBER_NAME_CHECKED(FChopItTerrainSettings, Seed))
			checkf(StaticStruct()->FindPropertyByName(It->GetFName()),
				TEXT("Terrain profile is missing runtime setting %s"), *It->GetName());
	CopyFrom(FChopItTerrainSettings());
}

void FChopItTerrainProfile::CopyFrom(const FChopItTerrainSettings& Source)
{
	TransferMatchingProperties(FChopItTerrainSettings::StaticStruct(), &Source, StaticStruct(), this);
}

FChopItTerrainSettings FChopItTerrainProfile::WithSeed(int32 Seed) const
{
	FChopItTerrainSettings Result;
	TransferMatchingProperties(StaticStruct(), this, FChopItTerrainSettings::StaticStruct(), &Result);
	Result.Seed = Seed;
	return Result;
}

bool UChopItTerrainDataAsset::GenerateOnActor(AChopItTerrainActor* Actor, int32 RuntimeSeed, FString& OutError) const
{
	OutError.Reset();
	if (!IsValid(Actor)) { OutError = TEXT("Choose a terrain actor before generating from this asset."); return false; }
	const FChopItTerrainSettings NewTerrain = MakeSettings(RuntimeSeed);
	if (!NewTerrain.Validate(OutError)) return false;
	const FChopItTerrainSettings PreviousTerrain = Actor->TerrainSettings;
	const FChopItTerrainMeshSettings PreviousMesh = Actor->MeshSettings;
	Actor->TerrainSettings = NewTerrain;
	Actor->MeshSettings = Mesh;
	if (Actor->RebuildTerrain(OutError)) return true;
	Actor->TerrainSettings = PreviousTerrain;
	Actor->MeshSettings = PreviousMesh;
	return false;
}

AChopItTerrainActor* UChopItTerrainDataAsset::SpawnTerrain(const UObject* WorldContextObject,
	FTransform Transform, int32 RuntimeSeed, FString& OutError) const
{
	return SpawnTerrainWithSettings(WorldContextObject, Transform, MakeSettings(RuntimeSeed), OutError);
}

AChopItTerrainActor* UChopItTerrainDataAsset::SpawnTerrainWithSettings(const UObject* WorldContextObject,
	FTransform Transform, const FChopItTerrainSettings& NewTerrain, FString& OutError) const
{
	OutError.Reset();
	if (!NewTerrain.Validate(OutError)) return nullptr;
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World) { OutError = TEXT("A world is required to spawn terrain."); return nullptr; }
	AChopItTerrainActor* Actor = World->SpawnActorDeferred<AChopItTerrainActor>(
		AChopItTerrainActor::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Actor) { OutError = TEXT("Could not spawn terrain actor."); return nullptr; }
	Actor->TerrainSettings = NewTerrain;
	Actor->MeshSettings = Mesh;
	Actor->FinishSpawning(Transform);
	if (!Actor->GetHeightField().IsValid())
	{
		OutError = TEXT("Terrain generation failed; check the Unreal log for details.");
		Actor->Destroy();
		return nullptr;
	}
	return Actor;
}
