#include "ChopItCollision.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Harvest/ChopItLogPickup.h"
#include "Harvest/ChopItTree.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Player/ChopItCharacter.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase3CargoTransferTest,
	"ChopIt.Phase3.CargoTransfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase3CargoTransferTest::RunTest(const FString& Parameters)
{
	UChopItWoodCargoComponent* Cargo = NewObject<UChopItWoodCargoComponent>();
	TestNotNull(TEXT("Cargo component exists"), Cargo);
	TestEqual(TEXT("Baseline capacity"), Cargo->GetCapacity(), 12);
	TestEqual(TEXT("Cargo starts empty"), Cargo->GetCurrentWood(), 0);
	TestFalse(TEXT("Cargo component has no Tick"), Cargo->PrimaryComponentTick.bCanEverTick);

	const FChopItWoodTransferResult FirstAdd = Cargo->TryAddWood(10);
	TestEqual(TEXT("First add transfers every requested unit"), FirstAdd.Transferred, 10);
	TestEqual(TEXT("First add has no remainder"), FirstAdd.Remainder, 0);
	TestEqual(TEXT("Cargo stores ten units"), Cargo->GetCurrentWood(), 10);

	const FChopItWoodTransferResult Overflow = Cargo->TryAddWood(5);
	TestEqual(TEXT("Overflow only fills available slots"), Overflow.Transferred, 2);
	TestEqual(TEXT("Overflow preserves three units as remainder"), Overflow.Remainder, 3);
	TestEqual(TEXT("Cargo never exceeds capacity"), Cargo->GetCurrentWood(), 12);
	TestFalse(TEXT("Capacity cannot shrink below current cargo"), Cargo->SetCapacity(11));

	const FChopItWoodTransferResult Removal = Cargo->TryRemoveWood(20);
	TestEqual(TEXT("Removal transfers all stored units"), Removal.Transferred, 12);
	TestEqual(TEXT("Removal reports unfulfilled units"), Removal.Remainder, 8);
	TestEqual(TEXT("Cargo returns to empty"), Cargo->GetCurrentWood(), 0);
	TestTrue(TEXT("Empty cargo may resize"), Cargo->SetCapacity(3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase3HarvestDefaultsTest,
	"ChopIt.Phase3.HarvestDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase3HarvestDefaultsTest::RunTest(const FString& Parameters)
{
	const AChopItTree* Tree = GetDefault<AChopItTree>();
	TestNotNull(TEXT("Tree CDO exists"), Tree);
	if (Tree)
	{
		TestFalse(TEXT("Trees do not Tick"), Tree->PrimaryActorTick.bCanEverTick);
		TestEqual(TEXT("Trees begin standing"), Tree->GetHarvestState(), EChopItTreeHarvestState::Standing);
		TestFalse(TEXT("Trees begin without a reward"), Tree->HasSpawnedReward());
		TestNotNull(TEXT("Tree has health"), Tree->GetHealthComponent());
		TestNotNull(TEXT("Tree has a physics root"), Tree->GetPhysicsRoot());
		TestNotNull(TEXT("Tree has a dedicated crown collision volume"), Tree->GetCrownCollision());
		if (Tree->GetPhysicsRoot())
		{
			TestFalse(TEXT("Standing tree physics starts disabled"), Tree->GetPhysicsRoot()->IsSimulatingPhysics());
			TestEqual(TEXT("Standing tree supports physics collision"), Tree->GetPhysicsRoot()->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
			TestEqual(TEXT("Tree physics never pushes the camera"), Tree->GetPhysicsRoot()->GetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid), ECR_Ignore);
		}
		for (const UStaticMeshComponent* VisualMesh : { Tree->GetTrunkMesh(), Tree->GetCrownMesh() })
		{
			TestNotNull(TEXT("Tree visual mesh exists"), VisualMesh);
			if (VisualMesh)
			{
				TestEqual(TEXT("Tree visual is query-only"), VisualMesh->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
				TestEqual(TEXT("Tree visual ignores camera push"), VisualMesh->GetCollisionResponseToChannel(ChopItCollisionChannels::CameraSolid), ECR_Ignore);
				TestEqual(TEXT("Tree visual participates in transparency"), VisualMesh->GetCollisionResponseToChannel(ChopItCollisionChannels::CameraOcclusion), ECR_Block);
			}
		}
		if (Tree->GetCrownCollision())
		{
			TestEqual(TEXT("Crown collision is query-only"), Tree->GetCrownCollision()->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
			TestEqual(TEXT("Crown detects the static world"), Tree->GetCrownCollision()->GetCollisionResponseToChannel(ECC_WorldStatic), ECR_Block);
			TestEqual(TEXT("Crown detects other harvestable trees"), Tree->GetCrownCollision()->GetCollisionResponseToChannel(ChopItCollisionChannels::Harvestable), ECR_Block);
			TestEqual(TEXT("Harvest volume does not steal camera transparency hits"), Tree->GetCrownCollision()->GetCollisionResponseToChannel(ChopItCollisionChannels::CameraOcclusion), ECR_Ignore);
		}
	}

	const AChopItLogPickup* Pickup = GetDefault<AChopItLogPickup>();
	TestNotNull(TEXT("Log pickup CDO exists"), Pickup);
	if (Pickup)
	{
		TestFalse(TEXT("Log pickups do not Tick"), Pickup->PrimaryActorTick.bCanEverTick);
		TestEqual(TEXT("Default log reward is integral"), Pickup->GetWoodUnits(), 3);
		TestNotNull(TEXT("Pickup has a dedicated physics body"), Pickup->GetPhysicsBody());
		TestEqual(TEXT("Pickup physics body is the actor root"), Pickup->GetRootComponent(), static_cast<USceneComponent*>(Pickup->GetPhysicsBody()));
		if (Pickup->GetPhysicsBody())
		{
			TestEqual(TEXT("Pickup physics body blocks the static world"), Pickup->GetPhysicsBody()->GetCollisionResponseToChannel(ECC_WorldStatic), ECR_Block);
			TestEqual(TEXT("Pickup physics body supports simulation"), Pickup->GetPhysicsBody()->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
		}
		TestNotNull(TEXT("Pickup has magnet overlap sphere"), Pickup->GetMagnetSphere());
		if (Pickup->GetMagnetSphere())
		{
			TestEqual(TEXT("Pickup collision is query-only"), Pickup->GetMagnetSphere()->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
			TestEqual(TEXT("Pickup overlaps pawns"), Pickup->GetMagnetSphere()->GetCollisionResponseToChannel(ECC_Pawn), ECR_Overlap);
		}
	}

	const AChopItCharacter* Character = GetDefault<AChopItCharacter>();
	TestNotNull(TEXT("Player owns wood cargo"), Character ? Character->GetWoodCargoComponent() : nullptr);
	TestNotNull(TEXT("Legacy world label remains safely hidden during HUD migration"), Character ? Character->GetWoodCargoLabel() : nullptr);
	TestFalse(TEXT("Player world label is not rendered"), Character && Character->GetWoodCargoLabel()
		? Character->GetWoodCargoLabel()->IsVisible() : true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItPhase3AssetsAndMapTest,
	"ChopIt.Phase3.AssetsAndMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhase3AssetsAndMapTest::RunTest(const FString& Parameters)
{
	UClass* TreeBlueprintClass = LoadClass<AChopItTree>(nullptr, TEXT("/Game/ChopIt/World/Forest/Trees/BP_Tree_Basic.BP_Tree_Basic_C"));
	UClass* PickupBlueprintClass = LoadClass<AChopItLogPickup>(nullptr, TEXT("/Game/ChopIt/World/Pickups/BP_LogPickup.BP_LogPickup_C"));
	TestNotNull(TEXT("BP_Tree_Basic exists"), TreeBlueprintClass);
	TestNotNull(TEXT("BP_LogPickup exists"), PickupBlueprintClass);

	const FString HarvestMapPackage = TEXT("/Game/ChopIt/World/Maps/L_Test_Harvest");
	TestTrue(TEXT("Harvest test map package exists"), FPackageName::DoesPackageExist(HarvestMapPackage));
	UPackage* Package = LoadPackage(nullptr, *HarvestMapPackage, LOAD_None);
	TestNotNull(TEXT("Harvest map package loads"), Package);
	if (!Package)
	{
		return false;
	}

	UWorld* World = FindObject<UWorld>(Package, TEXT("L_Test_Harvest"));
	TestNotNull(TEXT("Harvest map world exists"), World);
	if (!World)
	{
		return false;
	}

	int32 TreeCount = 0;
	bool bHasTreeInAttackRange = false;
	for (AActor* Actor : World->PersistentLevel->Actors)
	{
		if (AChopItTree* Tree = Cast<AChopItTree>(Actor))
		{
			++TreeCount;
			bHasTreeInAttackRange |= FVector::DistSquared2D(Tree->GetActorLocation(), FVector(750.0f, 0.0f, 0.0f)) <= FMath::Square(500.0f);
		}
	}
	TestEqual(TEXT("Harvest map has twenty trees"), TreeCount, 20);
	TestTrue(TEXT("Harvest map places one tree inside the initial axe arc"), bHasTreeInAttackRange);
	return true;
}

#endif
