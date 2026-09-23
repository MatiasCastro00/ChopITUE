#include "Economy/ChopItChainDefinition.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "ChopItCollision.h"

namespace
{
	bool SaveChainAsset(UObject* Asset)
	{
		Asset->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString Filename = FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		if (IFileManager::Get().IsReadOnly(*Filename) && !FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Filename, false)) return false;
		return UPackage::SavePackage(Asset->GetOutermost(), Asset,
			*Filename, Args);
	}
	UStaticMesh* CreateChainLink()
	{
		const TCHAR* Path = TEXT("/Game/ChopIt/World/ChainLab/SM_ChainLink_V2");
		if (UStaticMesh* Existing = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/ChopIt/World/ChainLab/SM_ChainLink_V2.SM_ChainLink_V2")))
			return Existing;
		UPackage* Package = CreatePackage(Path);
		UStaticMesh* Mesh = NewObject<UStaticMesh>(Package, TEXT("SM_ChainLink_V2"), RF_Public | RF_Standalone);
		FMeshDescription Description;
		FStaticMeshAttributes Attributes(Description);
		Attributes.Register();
		auto Vertices = Attributes.GetVertexPositions();
		auto Normals = Attributes.GetVertexInstanceNormals();
		auto UVs = Attributes.GetVertexInstanceUVs();
		UVs.SetNumChannels(1);
		constexpr int32 Around = 20, Tube = 6;
		TArray<FVertexID> IDs;
		for (int32 A = 0; A < Around; ++A)
		{
			const float Angle = 2 * PI * A / Around;
			const FVector3f Radial(FMath::Cos(Angle), 0, FMath::Sin(Angle));
			const FVector3f Centre(28 * FMath::Cos(Angle), 0, 48 * FMath::Sin(Angle));
			for (int32 B = 0; B < Tube; ++B)
			{
				const float T = 2 * PI * B / Tube;
				const FVertexID V = Description.CreateVertex();
				Vertices[V] = Centre + 6 * (Radial * FMath::Cos(T) + FVector3f(0,1,0) * FMath::Sin(T));
				IDs.Add(V);
			}
		}
		const FPolygonGroupID Group = Description.CreatePolygonGroup();
		for (int32 A = 0; A < Around; ++A)
		for (int32 B = 0; B < Tube; ++B)
		{
			const int32 Indices[] = {A * Tube + B, ((A + 1) % Around) * Tube + B,
				((A + 1) % Around) * Tube + (B + 1) % Tube, A * Tube + (B + 1) % Tube};
			TArray<FVertexInstanceID> Corners;
			for (int32 K = 0; K < 4; ++K)
			{
				const FVertexInstanceID V = Description.CreateVertexInstance(IDs[Indices[K]]);
				const int32 RA = Indices[K] / Tube, RB = Indices[K] % Tube;
				const float Angle = 2 * PI * RA / Around, T = 2 * PI * RB / Tube;
				Normals[V] = FVector3f(FMath::Cos(Angle) * FMath::Cos(T), FMath::Sin(T), FMath::Sin(Angle) * FMath::Cos(T));
				UVs.Set(V, 0, FVector2f(static_cast<float>(RA)/Around, static_cast<float>(RB)/Tube));
				Corners.Add(V);
			}
			Description.CreatePolygon(Group, Corners);
		}
		Mesh->GetStaticMaterials().Add(FStaticMaterial());
		Mesh->AddSourceModel();
		Mesh->CreateMeshDescription(0, MoveTemp(Description));
		Mesh->CommitMeshDescription(0);
		Mesh->Build(false);
		FAssetRegistryModule::AssetCreated(Mesh);
		return SaveChainAsset(Mesh) ? Mesh : nullptr;
	}
}

bool MigrateChainV2Assets()
{
	UChopItChainDefinition* Definition = LoadObject<UChopItChainDefinition>(nullptr,
		TEXT("/Game/ChopIt/World/ChainLab/DA_Chain_Default.DA_Chain_Default"));
	if (!Definition) return false;
	Definition->MigrateToV2();
	if (!Definition->ChainLinkMesh || Definition->ChainLinkMesh->GetPathName().StartsWith(TEXT("/Engine/BasicShapes/")))
	{
		Definition->ChainLinkMesh = CreateChainLink();
		if (!Definition->ChainLinkMesh) return false;
	}
	if (!SaveChainAsset(Definition)) return false;
	UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(TEXT("/Game/ChopIt/World/Maps/L_Test_ChainLab"));
	if (!World) return false;
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube) return false;
	// Additive, idempotent lab migration. No other maps or presets are regenerated.
	struct FSpec { const TCHAR* Name; FVector Position; FVector Scale; float Mass; };
	const FSpec Specs[] = {
		{TEXT("ChainV2_Light_10kg"), FVector(1350,1400,70), FVector(1,1,1), 10},
		{TEXT("ChainV2_Heavy_200kg"), FVector(1650,1400,70), FVector(1,1,1), 200},
		{TEXT("ChainV2_ThinWall"), FVector(-1000,-1000,150), FVector(0.02,3,3), 0}};
	for (const FSpec& S : Specs)
	{
		bool bExists = false;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
			if (It->GetActorLabel() == S.Name) { bExists = true; break; }
		if (bExists) continue;
		AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(S.Position, FRotator::ZeroRotator);
		Actor->SetActorLabel(S.Name);
		UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetWorldScale3D(S.Scale);
		Mesh->SetCollisionProfileName(TEXT("BlockAll"));
		Mesh->SetCollisionResponseToChannel(ChopItCollisionChannels::Chain, ECR_Block);
		if (S.Mass > 0)
		{
			Mesh->SetSimulatePhysics(true);
			Mesh->SetMassOverrideInKg(NAME_None, S.Mass, true);
		}
		else Mesh->SetMobility(EComponentMobility::Static);
	}
	const FString MapFile = FPackageName::LongPackageNameToFilename(TEXT("/Game/ChopIt/World/Maps/L_Test_ChainLab"), FPackageName::GetMapPackageExtension());
	if (IFileManager::Get().IsReadOnly(*MapFile) && !FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*MapFile, false)) return false;
	return UEditorLoadingAndSavingUtils::SaveMap(World, TEXT("/Game/ChopIt/World/Maps/L_Test_ChainLab"));
}
