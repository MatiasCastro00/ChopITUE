#include "Terrain/SChopItTerrainPreviewViewport.h"
#include "Terrain/ChopItTerrainEditorSettings.h"
#include "Terrain/ChopItTerrainActor.h"
#include "InputKeyEventArgs.h"
#include "Slate/SceneViewport.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"

FChopItTerrainPreviewViewportClient::FChopItTerrainPreviewViewportClient(FPreviewScene& Scene, const TWeakPtr<SEditorViewport>& Widget)
	: FEditorViewportClient(nullptr, &Scene, Widget)
{
	SetViewMode(VMI_Lit);
	SetRealtime(true);
	bSetListenerPosition = false;
	EngineShowFlags.SetGrid(false);
	EngineShowFlags.SetSelectionOutline(false);
	EngineShowFlags.DisableAdvancedFeatures();
	EngineShowFlags.SetLighting(true);
	ViewFOV = 55;
	bUseControllingActorViewInfo = true;
	ControllingActorViewInfo.FOV = ViewFOV;
	ControllingActorViewInfo.bConstrainAspectRatio = false;
	OverrideNearClipPlane(1.0f);
}

void FChopItTerrainPreviewViewportClient::FrameTerrain(const FBox& Bounds, bool bTop, bool bIsometric)
{
	Target = Bounds.GetCenter();
	Distance = FMath::Max(100.0, Bounds.GetExtent().Size() * 2.6);
	Rotation = bTop ? FRotator(-90, 0, 0) : FRotator(-35.264f, 45, 0);
	SetViewportType(bTop || bIsometric ? LVT_OrthoFreelook : LVT_Perspective);
	ControllingActorViewInfo.ProjectionMode = bTop || bIsometric ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
	SetOrthoZoom(float(Distance * 15));
	UpdateCamera();
}

void FChopItTerrainPreviewViewportClient::UpdateCamera()
{
	SetViewRotation(Rotation);
	SetViewLocation(Target - Rotation.Vector() * Distance);
	SetLookAtLocation(Target);
	ControllingActorViewInfo.Location = GetViewLocation();
	ControllingActorViewInfo.Rotation = GetViewRotation();
	ControllingActorViewInfo.OrthoWidth = float(Distance);
	Invalidate();
}

void FChopItTerrainPreviewViewportClient::Orbit(float X, float Y)
{
	Rotation.Yaw = FMath::UnwindDegrees(Rotation.Yaw + X * 0.3f);
	Rotation.Pitch = FMath::Clamp(Rotation.Pitch + Y * 0.3f, -89.0, 89.0);
	UpdateCamera();
}

void FChopItTerrainPreviewViewportClient::Pan(float X, float Y)
{
	const double Scale = Distance / 600.0;
	const FRotationMatrix Basis(Rotation);
	Target += (-Basis.GetUnitAxis(EAxis::Y) * X + Basis.GetUnitAxis(EAxis::Z) * Y) * Scale;
	UpdateCamera();
}

void FChopItTerrainPreviewViewportClient::Zoom(float Steps)
{
	const double Factor = FMath::Pow(0.85, Steps);
	Distance = FMath::Clamp(Distance * Factor, 10.0, 100000000.0);
	if (IsOrtho()) SetOrthoZoom(FMath::Clamp(float(GetOrthoZoom() * Factor), 150.0f, 1500000000.0f));
	UpdateCamera();
}

bool FChopItTerrainPreviewViewportClient::InputKey(const FInputKeyEventArgs& Args)
{
	if (Args.Key == EKeys::LeftMouseButton && Args.Event == IE_Pressed && bPaintMode && PaintField && OnTerrainClick)
	{
		const FViewportCursorLocation Cursor = GetCursorWorldLocationFromMousePos();
		const FVector Origin = Cursor.GetOrigin(), Direction = Cursor.GetDirection();
		if (FMath::Abs(Direction.Z) > 0.000001)
		{
			float Height = PaintField->GetMeanHeight();
			FVector Hit = Origin;
			for (int32 Iteration = 0; Iteration < 3; ++Iteration)
			{
				Hit = Origin + Direction * ((Height - Origin.Z) / Direction.Z);
				if (!PaintField->SampleLocalHeight(FVector2D(Hit.X, Hit.Y), Height)) return true;
			}
			OnTerrainClick(FVector2D(Hit.X, Hit.Y));
		}
		return true;
	}
	if (Args.Key == EKeys::LeftMouseButton || Args.Key == EKeys::RightMouseButton)
	{
		bOrbitDrag = Args.Event != IE_Released;
		return true;
	}
	if (Args.Key == EKeys::MiddleMouseButton)
	{
		bPanDrag = Args.Event != IE_Released;
		return true;
	}
	if (Args.Event == IE_Pressed && (Args.Key == EKeys::MouseScrollUp || Args.Key == EKeys::MouseScrollDown))
	{
		Zoom(Args.Key == EKeys::MouseScrollUp ? 1 : -1);
		return true;
	}
	return false;
}

bool FChopItTerrainPreviewViewportClient::InputAxis(const FInputKeyEventArgs& Args)
{
	if (Args.Key != EKeys::MouseX && Args.Key != EKeys::MouseY) return false;
	const float X = Args.Key == EKeys::MouseX ? Args.AmountDepressed : 0;
	const float Y = Args.Key == EKeys::MouseY ? Args.AmountDepressed : 0;
	if (bPanDrag) Pan(X, Y);
	else if (bOrbitDrag) Orbit(X, Y);
	return true;
}

void SChopItTerrainPreviewViewport::Construct(const FArguments& Args)
{
	Settings = Args._Settings;
	OnTerrainClick = Args._OnTerrainClick;
	Preview = MakeUnique<FChopItTerrainPreview>();
	SEditorViewport::Construct(SEditorViewport::FArguments());
}

SChopItTerrainPreviewViewport::~SChopItTerrainPreviewViewport()
{
	// The viewport client keeps a raw scene pointer; destroy it before the scene.
	if (PreviewClient) PreviewClient->Viewport = nullptr;
	if (SceneViewport) SceneViewport->SetViewportClient(TStrongPtrVariant<FViewportClient>(nullptr));
	Client.Reset();
	PreviewClient.Reset();
	Preview.Reset();
}

TSharedRef<FEditorViewportClient> SChopItTerrainPreviewViewport::MakeEditorViewportClient()
{
	PreviewClient = MakeShared<FChopItTerrainPreviewViewportClient>(Preview->GetScene(), SharedThis(this));
	PreviewClient->OnTerrainClick = [this](const FVector2D& XY) { if (OnTerrainClick) OnTerrainClick(XY); };
	PreviewClient->FrameTerrain(Preview->GetBounds());
	return PreviewClient.ToSharedRef();
}

void SChopItTerrainPreviewViewport::Tick(const FGeometry& Geometry, double Time, float DeltaTime)
{
	SEditorViewport::Tick(Geometry, Time, DeltaTime);
	if (!Settings.IsValid()) return;
	Preview->SetVisualization(Settings->PreviewMap);
	PreviewClient->bPaintMode = Settings->bPaintOnPreview && !Preview->IsBusy();
	Preview->Request(Settings->Terrain, Settings->Mesh, Settings->PreviewPolygonDetail, Time);
	if (Preview->Tick(Time))
	{
		if (AChopItTerrainActor* Actor = MaskActor.Get())
			if (FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&Settings->Terrain, &Actor->GetGeneratedSettings(), 0))
				Preview->SetMaskOverlay(Actor->GetHeightField());
		const FVector2D Size = Preview->GetHeightField()->GetSize();
		if (!bFramed || Size != LastSize)
		{
			PreviewClient->FrameTerrain(Preview->GetBounds());
			bFramed = true;
			LastSize = Size;
		}
		Invalidate();
	}
	PreviewClient->PaintField = Preview->GetHeightField();
	if (AChopItTerrainActor* Actor = MaskActor.Get())
		if (Actor->GetManualStrokeCount() != LastMaskStrokeCount
			&& FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&Settings->Terrain, &Actor->GetGeneratedSettings(), 0))
			ShowActorMasks(Actor->GetHeightField());
}

void SChopItTerrainPreviewViewport::ShowActorMasks(const FChopItHeightField& Field)
{
	Preview->SetMaskOverlay(Field);
	if (AChopItTerrainActor* Actor = MaskActor.Get()) LastMaskStrokeCount = Actor->GetManualStrokeCount();
	if (PreviewClient) PreviewClient->PaintField = Preview->GetHeightField();
	Invalidate();
}

void SChopItTerrainPreviewViewport::SetMaskActor(AChopItTerrainActor* Actor)
{
	MaskActor = Actor;
	LastMaskStrokeCount = -1;
	if (IsValid(Actor) && Settings.IsValid()
		&& FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&Settings->Terrain, &Actor->GetGeneratedSettings(), 0))
		ShowActorMasks(Actor->GetHeightField());
}

TSharedPtr<SWidget> SChopItTerrainPreviewViewport::BuildViewportToolbar()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(3)
		[
			SNew(SWrapBox).UseAllottedSize(true)
			+ SWrapBox::Slot().Padding(2)[SNew(SButton).Text(NSLOCTEXT("ChopItPreview", "Reset", "Reset Camera"))
				.OnClicked_Lambda([this]() { PreviewClient->FrameTerrain(Preview->GetBounds()); return FReply::Handled(); })]
			+ SWrapBox::Slot().Padding(2)[SNew(SButton).Text(NSLOCTEXT("ChopItPreview", "Top", "Top"))
				.OnClicked_Lambda([this]() { PreviewClient->FrameTerrain(Preview->GetBounds(), true); return FReply::Handled(); })]
			+ SWrapBox::Slot().Padding(2)[SNew(SButton).Text(NSLOCTEXT("ChopItPreview", "Iso", "Isometric"))
				.OnClicked_Lambda([this]() { PreviewClient->FrameTerrain(Preview->GetBounds(), false, true); return FReply::Handled(); })]
			+ SWrapBox::Slot().Padding(5, 2)[SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return PreviewClient->GetViewMode() == VMI_Wireframe ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { PreviewClient->SetViewMode(State == ECheckBoxState::Checked ? VMI_Wireframe : VMI_Lit); Invalidate(); })
				[SNew(STextBlock).Text(NSLOCTEXT("ChopItPreview", "Wire", "Wireframe"))]]
			+ SWrapBox::Slot().Padding(5, 2)[SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return Settings.IsValid() && Settings->Mesh.Shading == EChopItTerrainShading::Flat ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { if (Settings.IsValid()) { Settings->Mesh.Shading = State == ECheckBoxState::Checked ? EChopItTerrainShading::Flat : EChopItTerrainShading::Smooth; Settings->SaveConfig(); } })
				[SNew(STextBlock).Text(NSLOCTEXT("ChopItPreview", "Flat", "Flat Shading"))]]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5, 0)
		[SNew(STextBlock).AutoWrapText(true).Text(NSLOCTEXT("ChopItPreview", "Input", "Drag: orbit | Middle drag: pan | Wheel: zoom | Uncheck Flat for Smooth"))]
		+ SVerticalBox::Slot().AutoHeight().Padding(5, 3)
		[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]() { return Preview->GetStatus(); })]
		+ SVerticalBox::Slot().AutoHeight().Padding(5, 3)
		[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]() { return Preview->GetLegend(); })];
}
