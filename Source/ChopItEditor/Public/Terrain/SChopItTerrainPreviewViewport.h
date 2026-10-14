#pragma once

#include "SEditorViewport.h"
#include "Terrain/ChopItTerrainPreview.h"

class UChopItTerrainEditorSettings;
class AChopItTerrainActor;

class CHOPITEDITOR_API FChopItTerrainPreviewViewportClient : public FEditorViewportClient
{
public:
	FChopItTerrainPreviewViewportClient(FPreviewScene& Scene, const TWeakPtr<SEditorViewport>& Widget);
	virtual bool InputKey(const FInputKeyEventArgs& Args) override;
	virtual bool InputAxis(const FInputKeyEventArgs& Args) override;
	virtual FLinearColor GetBackgroundColor() const override { return FLinearColor(0.025f, 0.03f, 0.04f); }
	void FrameTerrain(const FBox& Bounds, bool bTop = false, bool bIsometric = false);
	void Orbit(float X, float Y);
	void Pan(float X, float Y);
	void Zoom(float Steps);
	TFunction<void(const FVector2D&)> OnTerrainClick;
	const FChopItHeightField* PaintField = nullptr;
	bool bPaintMode = false;
	FVector GetOrbitTarget() const { return Target; }
	double GetOrbitDistance() const { return Distance; }

private:
	FVector Target = FVector::ZeroVector;
	FRotator Rotation = FRotator(-35.264f, 45.0f, 0);
	double Distance = 40000;
	bool bOrbitDrag = false, bPanDrag = false;
	void UpdateCamera();
};

class CHOPITEDITOR_API SChopItTerrainPreviewViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SChopItTerrainPreviewViewport) {}
		SLATE_ARGUMENT(UChopItTerrainEditorSettings*, Settings)
		SLATE_ARGUMENT(TFunction<void(const FVector2D&)>, OnTerrainClick)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual ~SChopItTerrainPreviewViewport() override;
	virtual void Tick(const FGeometry& Geometry, double Time, float DeltaTime) override;
	void ShowActorMasks(const FChopItHeightField& Field);
	void SetMaskActor(AChopItTerrainActor* Actor);

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> BuildViewportToolbar() override;

private:
	TWeakObjectPtr<UChopItTerrainEditorSettings> Settings;
	TWeakObjectPtr<AChopItTerrainActor> MaskActor;
	int32 LastMaskStrokeCount = -1;
	TFunction<void(const FVector2D&)> OnTerrainClick;
	TUniquePtr<FChopItTerrainPreview> Preview;
	TSharedPtr<FChopItTerrainPreviewViewportClient> PreviewClient;
	bool bFramed = false;
	FVector2D LastSize = FVector2D::ZeroVector;
};
