#include "Terrain/ChopItTerrainMeshBuilder.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/MeshNormals.h"

using namespace UE::Geometry;

FIntPoint FChopItTerrainMeshBuilder::GetCellCounts(const FVector2D& Size, int32 PolygonDetail)
{
	if (!FMath::IsFinite(Size.X) || !FMath::IsFinite(Size.Y) || Size.X <= 0.0 || Size.Y <= 0.0) return FIntPoint::ZeroValue;
	const int32 Longest = FMath::RoundToInt(8.0 * FMath::Pow(32.0, FMath::Clamp(PolygonDetail, 0, 100) / 100.0));
	const double LongestSize = FMath::Max(Size.X, Size.Y);
	return FIntPoint(FMath::Max(1, FMath::RoundToInt(Longest * (Size.X / LongestSize))),
		FMath::Max(1, FMath::RoundToInt(Longest * (Size.Y / LongestSize))));
}

bool FChopItTerrainMeshBuilder::Build(const FChopItHeightField& Field, const FChopItTerrainMeshSettings& Settings,
	TArray<FChopItTerrainMeshChunk>& OutChunks, FString& OutError)
{
	OutError.Reset();
	if (!Field.IsValid() || Settings.PolygonDetail < 0 || Settings.PolygonDetail > 100
		|| Settings.CellsPerChunk < 8 || Settings.CellsPerChunk > 128
		|| (Settings.Shading != EChopItTerrainShading::Smooth && Settings.Shading != EChopItTerrainShading::Flat))
	{
		OutError = TEXT("Mesh requires a valid heightmap, detail 0-100, chunk size 8-128 and a supported shading mode.");
		return false;
	}
	const FIntPoint Cells = GetCellCounts(Field.GetSize(), Settings.PolygonDetail);
	const int32 Stride = Cells.X + 1;
	TArray<FVector3d> Positions, Normals;
	Positions.SetNumUninitialized(Stride * (Cells.Y + 1));
	Normals.Init(FVector3d::ZeroVector, Positions.Num());
	for (int32 Y = 0; Y <= Cells.Y; ++Y)
	{
		for (int32 X = 0; X <= Cells.X; ++X)
		{
			const FVector2D XY((double(X) / Cells.X - 0.5) * Field.GetSize().X, (double(Y) / Cells.Y - 0.5) * Field.GetSize().Y);
			float Height;
			if (!Field.SampleLocalHeight(XY, Height))
			{
				OutError = TEXT("Unable to sample the heightmap while constructing the mesh.");
				return false;
			}
			Positions[X + Y * Stride] = FVector3d(XY.X, XY.Y, Height);
		}
	}
	// Area-weighted normals over the entire grid, before partitioning: identical on both sides of each seam.
	const auto AccumulateNormal = [&](int32 A, int32 B, int32 C)
	{
		// GeometryCore uses Unreal's clockwise winding (reversed cross product).
		const FVector3d N = FVector3d::CrossProduct(Positions[C] - Positions[A], Positions[B] - Positions[A]);
		Normals[A] += N; Normals[B] += N; Normals[C] += N;
	};
	for (int32 Y = 0; Y < Cells.Y; ++Y)
	{
		for (int32 X = 0; X < Cells.X; ++X)
		{
			const int32 A = X + Y * Stride, B = A + 1, C = A + Stride, D = C + 1;
			AccumulateNormal(A, D, B);
			AccumulateNormal(A, C, D);
		}
	}
	for (FVector3d& N : Normals) N.Normalize();

	TArray<FChopItTerrainMeshChunk> Result;
	Result.Reserve(FMath::DivideAndRoundUp(Cells.X, Settings.CellsPerChunk) * FMath::DivideAndRoundUp(Cells.Y, Settings.CellsPerChunk));
	for (int32 StartY = 0; StartY < Cells.Y; StartY += Settings.CellsPerChunk)
	{
		for (int32 StartX = 0; StartX < Cells.X; StartX += Settings.CellsPerChunk)
		{
			FChopItTerrainMeshChunk& Chunk = Result.AddDefaulted_GetRef();
			Chunk.FirstCell = FIntPoint(StartX, StartY);
			Chunk.CellCount = FIntPoint(FMath::Min(Settings.CellsPerChunk, Cells.X - StartX), FMath::Min(Settings.CellsPerChunk, Cells.Y - StartY));
			FDynamicMesh3& Mesh = *Chunk.Mesh;
			Mesh.EnableAttributes();
			auto* UVs = Mesh.Attributes()->PrimaryUV();
			auto* NormalOverlay = Mesh.Attributes()->PrimaryNormals();
			TArray<int32> UVIds, NormalIds;
			const int32 VertexCount = (Chunk.CellCount.X + 1) * (Chunk.CellCount.Y + 1);
			UVIds.Reserve(VertexCount);
			if (Settings.Shading == EChopItTerrainShading::Smooth) NormalIds.Reserve(VertexCount);
			for (int32 Y = 0; Y <= Chunk.CellCount.Y; ++Y)
			{
				for (int32 X = 0; X <= Chunk.CellCount.X; ++X)
				{
					const int32 Global = StartX + X + (StartY + Y) * Stride;
					Mesh.AppendVertex(Positions[Global]);
					// One texture tile per 1000 cm; coordinates stay continuous between chunks.
					UVIds.Add(UVs->AppendElement(FVector2f(float((Positions[Global].X + Field.GetSize().X * 0.5) / 1000.0), float((Positions[Global].Y + Field.GetSize().Y * 0.5) / 1000.0))));
					if (Settings.Shading == EChopItTerrainShading::Smooth) NormalIds.Add(NormalOverlay->AppendElement(FVector3f(Normals[Global])));
				}
			}
			const auto AddTriangle = [&](int32 A, int32 B, int32 C)
			{
				const int32 T = Mesh.AppendTriangle(A, B, C);
				UVs->SetTriangle(T, FIndex3i(UVIds[A], UVIds[B], UVIds[C]));
				if (Settings.Shading == EChopItTerrainShading::Smooth) NormalOverlay->SetTriangle(T, FIndex3i(NormalIds[A], NormalIds[B], NormalIds[C]));
			};
			const int32 LocalStride = Chunk.CellCount.X + 1;
			for (int32 Y = 0; Y < Chunk.CellCount.Y; ++Y)
			{
				for (int32 X = 0; X < Chunk.CellCount.X; ++X)
				{
					const int32 A = X + Y * LocalStride, B = A + 1, C = A + LocalStride, D = C + 1;
					AddTriangle(A, D, B);
					AddTriangle(A, C, D);
				}
			}
			if (Settings.Shading == EChopItTerrainShading::Flat) FMeshNormals::InitializeOverlayToPerTriangleNormals(NormalOverlay);
		}
	}
	OutChunks = MoveTemp(Result);
	return true;
}
