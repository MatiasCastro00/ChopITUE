#include "Terrain/ChopItTerrainTestGameMode.h"
#include "Terrain/ChopItTerrainProjectSettings.h"
#include "Terrain/ChopItTerrainActor.h"
#include "Terrain/ChopItTerrainMeshBuilder.h"
#include "GameFramework/DefaultPawn.h"
#include "Misc/Guid.h"

AChopItTerrainTestGameMode::AChopItTerrainTestGameMode()
{
	DefaultPawnClass = ADefaultPawn::StaticClass();
}

void AChopItTerrainTestGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	if (!bGenerateTerrainAtRuntime) return;
	UChopItTerrainDataAsset* Asset = GetDefault<UChopItTerrainProjectSettings>()->DefaultTerrain.LoadSynchronous();
	if (!Asset)
	{
		ErrorMessage = TEXT("Choose a default terrain Data Asset in Project Settings > Game > ChopIt Terrain.");
		UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMessage);
		return;
	}
	// Every Play creates a fresh run, even if the editor retains old URL options.
	static TOptional<int32> PreviousRunSeed;
	RuntimeSeed = static_cast<int32>(FGuid::NewGuid().A);
	if (PreviousRunSeed.IsSet() && RuntimeSeed == PreviousRunSeed.GetValue())
	{
		RuntimeSeed = RuntimeSeed == MAX_int32 ? MIN_int32 : RuntimeSeed + 1;
	}
	PreviousRunSeed = RuntimeSeed;
	FChopItTerrainSettings RunSettings = Asset->MakeSettings(RuntimeSeed);
	RunSettings.SpawnFlatCenter = FVector2D::ZeroVector;
	const FIntPoint MeshCells = FChopItTerrainMeshBuilder::GetCellCounts(
		FVector2D(RunSettings.Width, RunSettings.Length), Asset->Mesh.PolygonDetail);
	const double MeshStepX = RunSettings.Width / MeshCells.X;
	const double MeshStepY = RunSettings.Length / MeshCells.Y;
	RunSettings.SpawnFlatRadius = FMath::Max(RunSettings.SpawnFlatRadius, 1000.0f)
		+ float(FMath::Sqrt(MeshStepX * MeshStepX + MeshStepY * MeshStepY));
	RunSettings.SpawnFlatBlendWidth = FMath::Max(RunSettings.SpawnFlatBlendWidth, 500.0f);
	GeneratedTerrain = Asset->SpawnTerrainWithSettings(this, FTransform::Identity, RunSettings, ErrorMessage);
	if (GeneratedTerrain)
	{
		UE_LOG(LogTemp, Display, TEXT("Terrain test generated: asset=%s seed=%d samples=%dx%d"),
			*Asset->GetPathName(), RuntimeSeed, GeneratedTerrain->TerrainSettings.SamplesX, GeneratedTerrain->TerrainSettings.SamplesY);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Terrain test generation failed: %s"), *ErrorMessage);
	}
}

void AChopItTerrainTestGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!GeneratedTerrain) { Super::RestartPlayer(NewPlayer); return; }
	const FChopItTerrainSettings& Terrain = GeneratedTerrain->TerrainSettings;
	const FVector Position(0.0, 0.0, Terrain.MaxHeight + FMath::Max(1500.0, FMath::Min(Terrain.Width, Terrain.Length) * 0.12));
	RestartPlayerAtTransform(NewPlayer, FTransform(FRotator(-30.0, 30.0, 0.0), Position));
}
