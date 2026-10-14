#include "Terrain/ChopItTerrainPreview.h"
#include "Terrain/ChopItTerrainGenerator.h"
#include "Async/Async.h"
#include "Components/DynamicMeshComponent.h"
#include "Materials/Material.h"
#include "UDynamicMesh.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

FChopItTerrainPreview::FChopItTerrainPreview()
	: Scene(FPreviewScene::ConstructionValues().SetCreatePhysicsScene(false).ShouldSimulatePhysics(false)
		.SetTransactional(false).SetLightRotation(FRotator(-35, -45, 0)).SetLightBrightness(3.0f).SetSkyBrightness(0.5f))
{
}

FChopItTerrainPreview::~FChopItTerrainPreview()
{
	// Closing a tab must not leave code executing after its editor module unloads.
	if (Future.IsValid()) Future.Wait();
	for (UDynamicMeshComponent* Component : Components)
	{
		Scene.RemoveComponent(Component);
		Component->DestroyComponent();
	}
}

void FChopItTerrainPreview::Request(const FChopItTerrainSettings& Terrain, const FChopItTerrainMeshSettings& Mesh, int32 Detail, double Now)
{
	FChopItTerrainMeshSettings PreviewMesh = Mesh;
	PreviewMesh.PolygonDetail = Detail;
	PreviewMesh.bEnableCollision = false;
	if (RequestedRevision && FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&Terrain, &RequestedTerrain, 0)
		&& FChopItTerrainMeshSettings::StaticStruct()->CompareScriptStruct(&PreviewMesh, &RequestedMesh, 0)) return;
	RequestedTerrain = Terrain;
	RequestedMesh = PreviewMesh;
	++RequestedRevision;
	LastChangeTime = Now;
	bPending = true;
	Error.Reset();
}

bool FChopItTerrainPreview::Tick(double Now)
{
	bool bPublished = false;
	if (Future.IsValid() && Future.IsReady())
	{
		FChopItTerrainPreviewResult Result = Future.Consume();
		// A stale result must never replace a more recent edit, even while it is debouncing.
		if (Result.Revision == RequestedRevision)
		{
			Error = Result.Error;
			if (Error.IsEmpty())
			{
				Publish(MoveTemp(Result));
				CachedTerrain = RequestedTerrain;
				bPublished = true;
			}
		}
	}
	if (bPending && !Future.IsValid() && Now - LastChangeTime >= 0.25)
	{
		bPending = false;
		const auto Terrain = RequestedTerrain;
		const auto Mesh = RequestedMesh;
		const uint64 Revision = RequestedRevision;
		auto CachedField = FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&Terrain, &CachedTerrain, 0) ? PublishedField : nullptr;
		Future = Async(EAsyncExecution::ThreadPool, [Terrain, Mesh, Revision, CachedField]()
		{
			FChopItTerrainPreviewResult Result;
			Result.Revision = Revision;
			Result.Field = CachedField;
			if (!Result.Field.IsValid())
			{
				auto Field = MakeShared<FChopItHeightField, ESPMode::ThreadSafe>();
				if (!FChopItTerrainGenerator::Generate(Terrain, *Field, Result.Error)) return Result;
				Result.Field = Field;
			}
			FChopItTerrainMeshBuilder::Build(*Result.Field, Mesh, Result.Chunks, Result.Error);
			return Result;
		});
	}
	return bPublished;
}

void FChopItTerrainPreview::Publish(FChopItTerrainPreviewResult&& Result)
{
	PublishedField = Result.Field;
	while (Components.Num() > Result.Chunks.Num())
	{
		UDynamicMeshComponent* Extra = Components.Pop();
		Scene.RemoveComponent(Extra);
		Extra->DestroyComponent();
	}
	for (int32 Index = 0; Index < Result.Chunks.Num(); ++Index)
	{
		if (Index >= Components.Num())
		{
			auto* Component = NewObject<UDynamicMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
			Component->SetDeferredCollisionUpdatesEnabled(true, false);
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Component->SetComplexAsSimpleCollisionEnabled(false, false);
			Component->SetCanEverAffectNavigation(false);
			Components.Add(Component);
			Scene.AddComponent(Component, FTransform::Identity);
		}
		UDynamicMeshComponent* Component = Components[Index];
		Component->SetTangentsType(EDynamicMeshComponentTangentsMode::AutoCalculated);
		Component->SetMesh(MoveTemp(*Result.Chunks[Index].Mesh));
		ApplyVisualization(Component);
	}
	PublishedRevision = Result.Revision;
	Scene.GetWorld()->SendAllEndOfFrameUpdates();
}

void FChopItTerrainPreview::SetVisualization(EChopItTerrainPreviewMode Mode)
{
	if (Mode == Visualization || uint8(Mode) > uint8(EChopItTerrainPreviewMode::WaterLevel)) return;
	Visualization = Mode;
	for (UDynamicMeshComponent* Component : Components) ApplyVisualization(Component);
	Scene.GetWorld()->SendAllEndOfFrameUpdates();
}

void FChopItTerrainPreview::SetMaskOverlay(const FChopItHeightField& Field)
{
	if (!PublishedField || Field.GetSamplesX() != PublishedField->GetSamplesX()
		|| Field.GetSamplesY() != PublishedField->GetSamplesY() || Field.GetSize() != PublishedField->GetSize()
		|| Field.GetHeights() != PublishedField->GetHeights()) return;
	PublishedField = MakeShared<FChopItHeightField, ESPMode::ThreadSafe>(Field);
	for (UDynamicMeshComponent* Component : Components) ApplyVisualization(Component);
	Scene.GetWorld()->SendAllEndOfFrameUpdates();
}

FLinearColor FChopItTerrainPreview::SampleVisualizationColor(const FChopItHeightField& Field, EChopItTerrainPreviewMode Mode, const FVector2D& XY)
{
	if (Mode == EChopItTerrainPreviewMode::Regions || Mode == EChopItTerrainPreviewMode::RegionBlend)
	{
		static const FLinearColor Palette[] = { FLinearColor(0.08f, 0.6f, 0.15f), FLinearColor(0.6f, 0.4f, 0.05f),
			FLinearColor(0.9f, 0.05f, 0.03f), FLinearColor(0.45f, 0.08f, 0.8f), FLinearColor(0.03f, 0.25f, 0.9f),
			FLinearColor(1.0f, 0.9f, 0.08f), FLinearColor(0.5f, 0.5f, 0.5f) };
		FChopItTerrainRegionBlend Blend;
		if (!Field.SampleLocalRegionBlend(XY, Blend)) return FLinearColor::Black;
		if (Mode == EChopItTerrainPreviewMode::Regions)
		{
			const int32 X = FMath::Clamp(FMath::RoundToInt((XY.X / Field.GetSize().X + 0.5) * (Field.GetSamplesX() - 1)), 0, Field.GetSamplesX() - 1);
			const int32 Y = FMath::Clamp(FMath::RoundToInt((XY.Y / Field.GetSize().Y + 0.5) * (Field.GetSamplesY() - 1)), 0, Field.GetSamplesY() - 1);
			return Palette[int32(Field.GetMaps().Regions[X + Y * Field.GetSamplesX()])];
		}
		FLinearColor Color(0, 0, 0, 0);
		for (int32 R = 0; R < FChopItTerrainRegionBlend::Count; ++R) Color += Palette[R] * Blend.Weights[R];
		Color.A = 1;
		return Color;
	}
	EChopItTerrainMap Map = EChopItTerrainMap::Height;
	switch (Mode)
	{
	case EChopItTerrainPreviewMode::Slope: Map = EChopItTerrainMap::Slope; break;
	case EChopItTerrainPreviewMode::Curvature: Map = EChopItTerrainMap::Curvature; break;
	case EChopItTerrainPreviewMode::Walkability: Map = EChopItTerrainMap::Walkability; break;
	case EChopItTerrainPreviewMode::Clearing: Map = EChopItTerrainMap::Clearing; break;
	case EChopItTerrainPreviewMode::Moisture: Map = EChopItTerrainMap::Moisture; break;
	case EChopItTerrainPreviewMode::VegetationDensity: Map = EChopItTerrainMap::VegetationDensity; break;
	default: break;
	}
	float Value = 0;
	if (!Field.SampleLocalMap(Map, XY, Value)) return FLinearColor::Black;
	if (Mode == EChopItTerrainPreviewMode::WaterLevel)
		return Value <= Field.GetWaterLevel() ? FLinearColor(0.02f, 0.25f, 1.0f) : FLinearColor(0.25f, 0.65f, 0.12f);
	if (Mode == EChopItTerrainPreviewMode::Height)
	{
		const float Range = Field.GetMaxHeight() - Field.GetMinHeight();
		const float T = Range > 0 ? (Value - Field.GetMinHeight()) / Range : 0.5f;
		const FLinearColor Middle(0.02f, 0.65f, 0.45f);
		return T <= 0.5f ? FMath::Lerp(FLinearColor(0.02f, 0.1f, 0.8f), Middle, T * 2.0f)
			: FMath::Lerp(Middle, FLinearColor(1.0f, 0.8f, 0.1f), (T - 0.5f) * 2.0f);
	}
	if (Mode == EChopItTerrainPreviewMode::Slope)
		return FMath::Lerp(FLinearColor(0.04f, 0.7f, 0.1f), FLinearColor(0.95f, 0.04f, 0.02f), FMath::Clamp(Value / 45.0f, 0.0f, 1.0f));
	if (Mode == EChopItTerrainPreviewMode::Curvature)
	{
		const float T = FMath::Abs(Value) / (FMath::Abs(Value) + 0.001f);
		return FMath::Lerp(FLinearColor::White, Value >= 0 ? FLinearColor(0.05f, 0.2f, 0.9f) : FLinearColor(1.0f, 0.2f, 0.02f), T);
	}
	return FLinearColor(Value, Value, Value, 1);
}

void FChopItTerrainPreview::ApplyVisualization(UDynamicMeshComponent* Component)
{
	const bool bSurface = Visualization == EChopItTerrainPreviewMode::Surface;
	const TCHAR* Path = bSurface ? TEXT("/Game/ChopIt/World/Blockout/Materials/MI_Ground.MI_Ground")
		: TEXT("/Engine/EngineDebugMaterials/VertexColorViewMode_ColorOnly.VertexColorViewMode_ColorOnly");
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, Path);
	Component->SetMaterial(0, Material ? Material : UMaterial::GetDefaultMaterial(MD_Surface));
	if (bSurface || !PublishedField) return;
	if (!Material) { Error = TEXT("Unable to load the engine vertex-color material for map preview."); return; }
	Component->GetDynamicMesh()->EditMesh([this](UE::Geometry::FDynamicMesh3& Mesh)
	{
		Mesh.Attributes()->DisablePrimaryColors();
		Mesh.Attributes()->EnablePrimaryColors();
		auto* Colors = Mesh.Attributes()->PrimaryColors();
		TArray<int32> Elements;
		Elements.SetNumUninitialized(Mesh.MaxVertexID());
		for (int32 V : Mesh.VertexIndicesItr())
		{
			const FVector3d P = Mesh.GetVertex(V);
			const FLinearColor Color = SampleVisualizationColor(*PublishedField, Visualization, FVector2D(P.X, P.Y));
			Elements[V] = Colors->AppendElement(FVector4f(Color.R, Color.G, Color.B, 1));
		}
		for (int32 T : Mesh.TriangleIndicesItr())
		{
			const UE::Geometry::FIndex3i V = Mesh.GetTriangle(T);
			Colors->SetTriangle(T, UE::Geometry::FIndex3i(Elements[V.A], Elements[V.B], Elements[V.C]));
		}
	});
}

FText FChopItTerrainPreview::GetLegend() const
{
	switch (Visualization)
	{
	case EChopItTerrainPreviewMode::WaterLevel: return FText::FromString(TEXT("Water level: blue = submerged | green = above water. Classification sampled at preview vertices; no water objects are generated."));
	case EChopItTerrainPreviewMode::Height: return FText::FromString(TEXT("Height: blue = actual minimum | teal = middle | yellow = actual maximum (cm)"));
	case EChopItTerrainPreviewMode::Slope: return FText::FromString(TEXT("Slope: green = 0 degrees | red = 45 degrees or steeper"));
	case EChopItTerrainPreviewMode::Curvature: return FText::FromString(TEXT("Curvature (1/cm): orange = convex | white = flat | blue = concave; half intensity at +/-0.001"));
	case EChopItTerrainPreviewMode::Regions: return FText::FromString(TEXT("Region Map: green = flat forest | ochre = rolling hills | red = steep hills | purple = valleys | blue = lowlands | yellow = clearings | gray = transitions. Dominant categories sampled at preview vertices."));
	case EChopItTerrainPreviewMode::RegionBlend: return FText::FromString(TEXT("Smooth memberships: green = flat forest | ochre = rolling hills | red = steep hills | purple = valleys | blue = lowlands | yellow = clearings | gray = transitions. Colors blend continuous weights."));
	case EChopItTerrainPreviewMode::Surface: return FText::GetEmpty();
	default: return FText::FromString(TEXT("Potential mask: black = 0 | white = 1. Colors sample the source heightmap at preview vertices."));
	}
}

FBox FChopItTerrainPreview::GetBounds() const
{
	if (!PublishedField) return FBox(FVector(-10000, -10000, -300), FVector(10000, 10000, 600));
	const FVector2D Half = PublishedField->GetSize() * 0.5;
	return FBox(FVector(-Half.X, -Half.Y, PublishedField->GetMinHeight()), FVector(Half.X, Half.Y, PublishedField->GetMaxHeight()));
}

FText FChopItTerrainPreview::GetStatus() const
{
	if (!Error.IsEmpty()) return FText::FromString(Error);
	if (IsBusy()) return NSLOCTEXT("ChopItTerrainPreview", "Updating", "Updating preview...");
	if (!PublishedField) return NSLOCTEXT("ChopItTerrainPreview", "Ready", "Preview ready");
	return FText::FromString(FString::Printf(TEXT("Preview ready | Water: %.1f cm | Approx. submerged area: %.1f%%"),
		PublishedField->GetWaterLevel(), PublishedField->GetSubmergedFraction() * 100.0));
}
