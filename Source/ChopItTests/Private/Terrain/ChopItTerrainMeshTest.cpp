#include "Terrain/ChopItTerrainActor.h"
#include "Terrain/ChopItTerrainGenerator.h"
#include "Terrain/ChopItTerrainMeshBuilder.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
using namespace UE::Geometry;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainMeshDataTest, "ChopIt.Terrain.Geometry.MeshData", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainMeshDataTest::RunTest(const FString& Parameters)
{
	FChopItTerrainSettings Settings;
	FChopItHeightField Field;
	FString Error;
	if (!TestTrue(TEXT("Source field"), FChopItTerrainGenerator::Generate(Settings, Field, Error))) return false;
	const TArray<float> OriginalHeights = Field.GetHeights();
	FChopItTerrainMeshSettings MeshSettings;
	TArray<FChopItTerrainMeshChunk> Chunks;
	int32 PreviousTriangles = 0;
	for (int32 Detail : { 0, 50, 100 })
	{
		MeshSettings.PolygonDetail = Detail;
		if (!TestTrue(TEXT("Build mesh"), FChopItTerrainMeshBuilder::Build(Field, MeshSettings, Chunks, Error))) return false;
		TMap<FVector3d, FVector3f> SharedNormals;
		TMap<FVector3d, FVector2f> SharedUVs;
		int32 Triangles = 0, SharedVertices = 0;
		bool bSamplesMatch = true, bNormalsMatch = true, bUVsMatch = true, bUpward = true;
		for (const auto& Chunk : Chunks)
		{
			const FDynamicMesh3& Mesh = *Chunk.Mesh;
			if (!TestTrue(TEXT("Valid manifold mesh with complete overlays"), Mesh.CheckValidity({}, EValidityCheckFailMode::ReturnOnly))) return false;
			Triangles += Mesh.TriangleCount();
			const auto* Normals = Mesh.Attributes()->PrimaryNormals();
			const auto* UVs = Mesh.Attributes()->PrimaryUV();
			for (int32 T : Mesh.TriangleIndicesItr())
			{
				bUpward &= Mesh.GetTriNormal(T).Z > 0.0;
				const FIndex3i V = Mesh.GetTriangle(T), N = Normals->GetTriangle(T), UV = UVs->GetTriangle(T);
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					const FVector3d P = Mesh.GetVertex(V[Corner]);
					const FVector3f Normal = Normals->GetElement(N[Corner]);
					const FVector2f TexCoord = UVs->GetElement(UV[Corner]);
					float Height;
					bSamplesMatch &= Field.SampleLocalHeight(FVector2D(P.X, P.Y), Height) && P.Z == Height;
					bUpward &= Normal.Z > 0.0f && FMath::Abs(Normal.Size() - 1.0f) < 0.0001f;
					if (const FVector3f* Previous = SharedNormals.Find(P))
					{
						bNormalsMatch &= *Previous == Normal;
						bUVsMatch &= SharedUVs[P] == TexCoord;
					}
					else { SharedNormals.Add(P, Normal); SharedUVs.Add(P, TexCoord); }
				}
			}
			// Adjacent chunks overlap in vertices only, never in cells/triangles.
			if (Chunk.FirstCell.X > 0) SharedVertices += Chunk.CellCount.Y + 1;
		}
		const FIntPoint Cells = FChopItTerrainMeshBuilder::GetCellCounts(Field.GetSize(), Detail);
		TestEqual(TEXT("No duplicated or missing faces"), Triangles, 2 * Cells.X * Cells.Y);
		TestTrue(TEXT("Detail increases triangle count"), Triangles > PreviousTriangles);
		PreviousTriangles = Triangles;
		TestTrue(TEXT("Every vertex samples the unchanged source field"), bSamplesMatch);
		TestTrue(TEXT("Normals point up and are normalized"), bUpward);
		TestTrue(TEXT("Smooth normals are identical at shared positions"), bNormalsMatch);
		TestTrue(TEXT("UVs remain continuous"), bUVsMatch);
		if (Detail == 100) TestTrue(TEXT("High detail exercises chunk boundaries"), SharedVertices > 0 && Chunks.Num() > 1);
	}
	TestTrue(TEXT("Mesh detail never mutates the heightmap"), Field.GetHeights() == OriginalHeights);

	MeshSettings.PolygonDetail = 50;
	MeshSettings.CellsPerChunk = 8;
	MeshSettings.Shading = EChopItTerrainShading::Flat;
	if (!FChopItTerrainMeshBuilder::Build(Field, MeshSettings, Chunks, Error)) return false;
	bool bFlatCorrect = true;
	for (const auto& Chunk : Chunks)
	{
		const auto* Normals = Chunk.Mesh->Attributes()->PrimaryNormals();
		for (int32 T : Chunk.Mesh->TriangleIndicesItr())
		{
			const FIndex3i N = Normals->GetTriangle(T);
			const FVector3f Face(Chunk.Mesh->GetTriNormal(T));
			for (int32 Corner = 0; Corner < 3; ++Corner) bFlatCorrect &= FVector3f::DotProduct(Normals->GetElement(N[Corner]), Face) > 0.9999f;
		}
	}
	TestTrue(TEXT("Flat normals match every triangle face"), bFlatCorrect);
	TestTrue(TEXT("Flat shading preserves the heightmap"), Field.GetHeights() == OriginalHeights);
	const int32 PreviousChunks = Chunks.Num();
	MeshSettings.PolygonDetail = 101;
	TestFalse(TEXT("Invalid detail is rejected"), FChopItTerrainMeshBuilder::Build(Field, MeshSettings, Chunks, Error));
	TestEqual(TEXT("Failure preserves geometry"), Chunks.Num(), PreviousChunks);
	TestFalse(TEXT("Invalid heightmap is rejected"), FChopItTerrainMeshBuilder::Build(FChopItHeightField(), FChopItTerrainMeshSettings(), Chunks, Error));
	const FIntPoint Thin = FChopItTerrainMeshBuilder::GetCellCounts(FVector2D(100.0, 10000000.0), 100);
	TestEqual(TEXT("Narrow dimension remains nonzero"), Thin.X, 1);
	TestEqual(TEXT("Longest side obeys maximum"), Thin.Y, 256);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainSurfaceTest, "ChopIt.Terrain.Geometry.SurfacePreservation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainSurfaceTest::RunTest(const FString& Parameters)
{
	FString Error;
	for (int32 Seed : { 1337, -57, 12 })
	{
		FChopItTerrainSettings Settings;
		Settings.Seed = Seed;
		FChopItHeightField Field;
		if (!FChopItTerrainGenerator::Generate(Settings, Field, Error)) return false;
		double LowError = 0.0;
		for (int32 Detail : { 0, 100 })
		{
			FChopItTerrainMeshSettings MeshSettings;
			MeshSettings.PolygonDetail = Detail;
			TArray<FChopItTerrainMeshChunk> Chunks;
			if (!FChopItTerrainMeshBuilder::Build(Field, MeshSettings, Chunks, Error)) return false;
			double SquaredError = 0.0;
			int32 Count = 0;
			for (const auto& Chunk : Chunks)
			{
				for (int32 T : Chunk.Mesh->TriangleIndicesItr())
				{
					const FIndex3i V = Chunk.Mesh->GetTriangle(T);
					const FVector3d P = (Chunk.Mesh->GetVertex(V.A) + Chunk.Mesh->GetVertex(V.B) + Chunk.Mesh->GetVertex(V.C)) / 3.0;
					float SourceHeight;
					if (!Field.SampleLocalHeight(FVector2D(P.X, P.Y), SourceHeight)) return false;
					SquaredError += FMath::Square(P.Z - SourceHeight);
					++Count;
				}
			}
			const double RMSError = FMath::Sqrt(SquaredError / Count);
			AddInfo(FString::Printf(TEXT("Seed %d detail %d: surface RMS error %.3f cm, height range %.3f cm"), Seed, Detail, RMSError, Field.GetMaxHeight() - Field.GetMinHeight()));
			if (Detail == 0)
			{
				LowError = RMSError;
				TestTrue(TEXT("Coarsest mesh preserves broad default relief within 15 percent RMS of its height range"), RMSError < 0.15 * (Field.GetMaxHeight() - Field.GetMinHeight()));
			}
			else TestTrue(TEXT("High detail improves agreement with the heightmap"), RMSError < LowError);
			MeshSettings.Shading = EChopItTerrainShading::Flat;
			TArray<FChopItTerrainMeshChunk> Flat;
			if (!FChopItTerrainMeshBuilder::Build(Field, MeshSettings, Flat, Error)) return false;
			bool bIdenticalGeometry = Flat.Num() == Chunks.Num();
			for (int32 Index = 0; bIdenticalGeometry && Index < Chunks.Num(); ++Index)
			{
				bIdenticalGeometry &= Flat[Index].Mesh->VertexCount() == Chunks[Index].Mesh->VertexCount() && Flat[Index].Mesh->TriangleCount() == Chunks[Index].Mesh->TriangleCount();
				for (int32 V : Chunks[Index].Mesh->VertexIndicesItr()) bIdenticalGeometry &= Flat[Index].Mesh->GetVertex(V) == Chunks[Index].Mesh->GetVertex(V);
				for (int32 T : Chunks[Index].Mesh->TriangleIndicesItr()) bIdenticalGeometry &= Flat[Index].Mesh->GetTriangle(T) == Chunks[Index].Mesh->GetTriangle(T);
			}
			TestTrue(TEXT("Flat and smooth shading use identical geometry"), bIdenticalGeometry);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItTerrainActorTest, "ChopIt.Terrain.Geometry.ActorCollision", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItTerrainActorTest::RunTest(const FString& Parameters)
{
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Test level"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	AChopItTerrainActor* Terrain = World->SpawnActor<AChopItTerrainActor>(FVector(120, -70, 300), FRotator(0, 17, 0));
	if (!TestNotNull(TEXT("Placeable actor"), Terrain)) return false;
	Terrain->TerrainSettings.SamplesX = 65;
	Terrain->TerrainSettings.SamplesY = 49;
	Terrain->TerrainSettings.Width = 8000.0;
	Terrain->TerrainSettings.Length = 6000.0;
	Terrain->TerrainSettings.HeightVariation = 0.0f;
	FString Error;
	if (!TestTrue(TEXT("Generate in level"), Terrain->RebuildTerrain(Error))) return false;
	UDynamicMeshComponent* First = Terrain->GetTerrainChunks()[0];
	const float* HeightData = Terrain->GetHeightField().GetHeights().GetData();
	const FVector Start(120, -70, 3000), End(120, -70, -3000);
	FHitResult Hit;
	TestTrue(TEXT("Simple trace hits triangle collision"), World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility));
	TestTrue(TEXT("Collision is at transformed height"), FMath::Abs(Hit.ImpactPoint.Z - 400.0) < 0.1);
	TestTrue(TEXT("Player sphere sweep hits terrain"), World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(30.0f)));
	TestTrue(TEXT("Hit belongs to terrain"), Hit.GetActor() == Terrain);
	Terrain->MeshSettings.PolygonDetail = 100;
	if (!TestTrue(TEXT("Increase mesh resolution"), Terrain->RebuildTerrain(Error))) return false;
	TestTrue(TEXT("Heightmap allocation is reused"), HeightData == Terrain->GetHeightField().GetHeights().GetData());
	TestTrue(TEXT("First component is reused"), First == Terrain->GetTerrainChunks()[0]);
	TestTrue(TEXT("Higher detail produces multiple chunks"), Terrain->GetTerrainChunks().Num() > 1);
	for (int32 Boundary = 1; Boundary < 4; ++Boundary)
	{
		const FVector P = Terrain->GetActorTransform().TransformPosition(FVector((Boundary / 4.0 - 0.5) * 8000.0, 37.5, 0.0));
		TestTrue(TEXT("Collision has no hole at chunk border"), World->LineTraceSingleByChannel(Hit, P + FVector(0, 0, 3000), P - FVector(0, 0, 3000), ECC_Visibility));
	}
	Terrain->TerrainSettings.TargetMeanHeight = 250.0f;
	if (!Terrain->RebuildTerrain(Error)) return false;
	TestTrue(TEXT("Regenerated collision still hits"), World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility));
	TestTrue(TEXT("Collision uses new heights"), FMath::Abs(Hit.ImpactPoint.Z - 550.0) < 0.1);
	Terrain->MeshSettings.PolygonDetail = 0;
	Terrain->MeshSettings.Shading = EChopItTerrainShading::Flat;
	if (!Terrain->RebuildTerrain(Error)) return false;
	TArray<UDynamicMeshComponent*> Components;
	Terrain->GetComponents(Components);
	TestEqual(TEXT("Reducing detail removes obsolete components"), Components.Num(), 1);
	TestEqual(TEXT("Owned array has no obsolete entries"), Terrain->GetTerrainChunks().Num(), 1);
	TestTrue(TEXT("Shading change preserves collision"), World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility));
	Terrain->MeshSettings.bEnableCollision = false;
	if (!Terrain->RebuildTerrain(Error)) return false;
	TestFalse(TEXT("Disabled collision does not hit"), World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility));
	Terrain->MeshSettings.bEnableCollision = true;
	if (!Terrain->RebuildTerrain(Error)) return false;
	TestTrue(TEXT("Collision can be enabled again"), World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility));
	for (int32 Iteration = 0; Iteration < 8; ++Iteration)
	{
		Terrain->TerrainSettings.TargetMeanHeight = 200 + Iteration * 5;
		Terrain->MeshSettings.PolygonDetail = Iteration % 2 ? 20 : 75;
		if (!TestTrue(TEXT("Repeated collision rebuild"), Terrain->RebuildTerrain(Error))) return false;
		CollectGarbage(RF_NoFlags);
		TestTrue(TEXT("Collision survives GC and repartition"), World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility));
		TestTrue(TEXT("Rebuilt collision uses latest height"), FMath::Abs(Hit.ImpactPoint.Z - (500 + Iteration * 5)) < 0.1);
	}
	Terrain->TerrainSettings.SamplesX = 0;
	TestFalse(TEXT("Invalid regeneration fails"), Terrain->RebuildTerrain(Error));
	TestTrue(TEXT("Failed regeneration keeps old collision"), World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility));
	int32 Actors = 0;
	for (const AActor* Actor : World->PersistentLevel->Actors) if (IsValid(Actor) && Actor->IsA<AChopItTerrainActor>()) ++Actors;
	TestEqual(TEXT("Repeated regeneration never duplicates actors"), Actors, 1);
	return true;
}
#endif
