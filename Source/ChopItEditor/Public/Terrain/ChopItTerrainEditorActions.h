#pragma once

#include "CoreMinimal.h"

class AChopItTerrainActor;
class UChopItTerrainEditorSettings;
class UWorld;
class UStaticMesh;
class UChopItTerrainDataAsset;

/** Small editor-only action layer shared by the tab and automation tests. */
class CHOPITEDITOR_API FChopItTerrainEditorActions
{
public:
	/** Creates a uniquely named mesh asset in local terrain coordinates; caller saves the dirty package. */
	static UStaticMesh* BakeMesh(const AChopItTerrainActor* Target, const FString& BasePackageName, FString& OutError);
	/** Opens Save As, copies the current controls except Seed and writes a runtime Data Asset. */
	static UChopItTerrainDataAsset* SaveAsDataAsset(const UChopItTerrainEditorSettings& Settings, FString& OutError);
	/** Creates an actor if Target is null; otherwise rebuilds Target. Failure preserves the existing actor. */
	static bool Generate(UWorld* World, AChopItTerrainActor*& Target, const UChopItTerrainEditorSettings& Settings, FString& OutError);
	/** Rebuilds only an existing target. */
	static bool Regenerate(UWorld* World, AChopItTerrainActor* Target, const UChopItTerrainEditorSettings& Settings, FString& OutError);
	static bool ResetDefaults(UChopItTerrainEditorSettings& Settings, FString& OutError);
	static bool RandomSeed(UChopItTerrainEditorSettings& Settings, FString& OutError);
};
