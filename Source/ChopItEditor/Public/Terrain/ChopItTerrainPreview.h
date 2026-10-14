#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "PreviewScene.h"
#include "Terrain/ChopItTerrainSettings.h"
#include "Terrain/ChopItTerrainMeshBuilder.h"
#include "Terrain/ChopItTerrainPreviewMode.h"

class UDynamicMeshComponent;

struct FChopItTerrainPreviewResult
{
	uint64 Revision = 0;
	TSharedPtr<const FChopItHeightField, ESPMode::ThreadSafe> Field;
	TArray<FChopItTerrainMeshChunk> Chunks;
	FString Error;
};

/** Owns a transient, physics-free scene. Only the pure height/mesh work runs off the game thread. */
class CHOPITEDITOR_API FChopItTerrainPreview
{
public:
	FChopItTerrainPreview();
	~FChopItTerrainPreview();
	void Request(const FChopItTerrainSettings& Terrain, const FChopItTerrainMeshSettings& Mesh, int32 Detail, double Now);
	/** Poll completion and start the latest debounced request. Returns true when new geometry was published. */
	bool Tick(double Now);
	FPreviewScene& GetScene() { return Scene; }
	const TArray<UDynamicMeshComponent*>& GetComponents() const { return Components; }
	const FChopItHeightField* GetHeightField() const { return PublishedField.Get(); }
	FBox GetBounds() const;
	FText GetStatus() const;
	void SetVisualization(EChopItTerrainPreviewMode Mode);
	void SetMaskOverlay(const FChopItHeightField& Field);
	EChopItTerrainPreviewMode GetVisualization() const { return Visualization; }
	FText GetLegend() const;
	static FLinearColor SampleVisualizationColor(const FChopItHeightField& Field, EChopItTerrainPreviewMode Mode, const FVector2D& XY);
	uint64 GetPublishedRevision() const { return PublishedRevision; }
	bool IsBusy() const { return Future.IsValid() || bPending; }

private:
	FPreviewScene Scene;
	TArray<UDynamicMeshComponent*> Components;
	FChopItTerrainSettings RequestedTerrain, CachedTerrain;
	FChopItTerrainMeshSettings RequestedMesh;
	TSharedPtr<const FChopItHeightField, ESPMode::ThreadSafe> PublishedField;
	TFuture<FChopItTerrainPreviewResult> Future;
	uint64 RequestedRevision = 0, PublishedRevision = 0;
	double LastChangeTime = 0;
	bool bPending = false;
	FString Error;
	EChopItTerrainPreviewMode Visualization = EChopItTerrainPreviewMode::Surface;
	void ApplyVisualization(UDynamicMeshComponent* Component);
	void Publish(FChopItTerrainPreviewResult&& Result);
};
