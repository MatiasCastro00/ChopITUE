#include "Misc/AutomationTest.h"

#include "Economy/ChopItChainDefinition.h"
#include "Economy/ChopItRopeComponent.h"
#include "Economy/ChopItTetherPathComponent.h"
#include "Economy/ChopItTetherReceiverComponent.h"
#include "Components/BoxComponent.h"
#include "Tests/AutomationEditorCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItRopeDeploymentPreservesShapeTest,
	"ChopIt.Chain.DeploymentPreservesShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItRopeDeploymentPreservesShapeTest::RunTest(const FString& Parameters)
{
	UChopItChainDefinition* Definition = NewObject<UChopItChainDefinition>();
	Definition->ChainLinkCount = 20;
	Definition->MaxChainLength = 1000.0f;
	Definition->CableSegmentsPerLink = 2;
	Definition->bCableWorldCollision = false;
	Definition->CableGravityScale = 1.0f;

	UChopItRopeComponent* Rope = NewObject<UChopItRopeComponent>();
	Rope->Configure(Definition);
	const FVector Start(0.0f, 0.0f, 100.0f);
	const FVector End(500.0f, 0.0f, 100.0f);
	Rope->InitializeRope(Start, End, 600.0f, nullptr);
	for (int32 Step = 0; Step < 30; ++Step)
	{
		Rope->Simulate(1.0f / 60.0f);
	}

	const TArray<FVector> BeforeDeployment = Rope->GetParticleLocations();
	TestTrue(TEXT("The rope has interior simulation particles"), BeforeDeployment.Num() > 3);
	TestTrue(TEXT("Gravity creates visible rope sag"), BeforeDeployment[BeforeDeployment.Num() / 2].Z < Start.Z - 1.0f);
	const FVector PreservedInteriorParticle = BeforeDeployment[1];

	Rope->SetRopeLength(800.0f);
	const TArray<FVector>& AfterDeployment = Rope->GetParticleLocations();
	const int32 AddedParticles = AfterDeployment.Num() - BeforeDeployment.Num();
	TestTrue(TEXT("Deploying rope adds particles"), AddedParticles > 0);
	TestTrue(
		TEXT("New particles are inserted at the machine without resetting the existing rope"),
		AfterDeployment.IsValidIndex(1 + AddedParticles)
			&& AfterDeployment[1 + AddedParticles].Equals(PreservedInteriorParticle, 0.01f));
	TestTrue(TEXT("Start remains pinned"), AfterDeployment[0].Equals(Start, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("End remains pinned"), AfterDeployment.Last().Equals(End, KINDA_SMALL_NUMBER));

	Rope->SetRopeLength(600.0f);
	const TArray<FVector>& AfterRetraction = Rope->GetParticleLocations();
	TestEqual(TEXT("Retracting removes only the emitted machine particles"), AfterRetraction.Num(), BeforeDeployment.Num());
	TestTrue(
		TEXT("Retraction preserves the previously simulated shape"),
		AfterRetraction[1].Equals(PreservedInteriorParticle, 0.01f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItRopeResistsStretchWithoutReboundTest,
	"ChopIt.Chain.ResistsStretchWithoutRebound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItRopeResistsStretchWithoutReboundTest::RunTest(const FString& Parameters)
{
	UChopItChainDefinition* Definition = NewObject<UChopItChainDefinition>();
	Definition->ChainLinkCount = 20;
	Definition->MaxChainLength = 1000.0f;
	Definition->CableSegmentsPerLink = 2;
	Definition->CableSolverIterations = 32;
	Definition->CableConstraintVelocityDamping = 0.9f;
	Definition->bCableWorldCollision = false;
	Definition->CableGravityScale = 0.0f;

	UChopItRopeComponent* Rope = NewObject<UChopItRopeComponent>();
	Rope->Configure(Definition);
	const FVector Start(0.0f, 0.0f, 100.0f);
	Rope->InitializeRope(Start, FVector(500.0f, 0.0f, 100.0f), 600.0f, nullptr);
	Rope->SetEndpoints(Start, FVector(590.0f, 0.0f, 100.0f));
	for (int32 Step = 0; Step < 10; ++Step)
	{
		Rope->Simulate(1.0f / 60.0f);
	}

	const TArray<FVector>& Points = Rope->GetParticleLocations();
	const float SegmentLength = Rope->GetRopeLength() / FMath::Max(1, Points.Num() - 1);
	float MaximumStretch = 0.0f;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		MaximumStretch = FMath::Max(
			MaximumStretch,
			FVector::Distance(Points[Index - 1], Points[Index]) - SegmentLength);
	}
	TestTrue(
		*FString::Printf(TEXT("A moving endpoint does not leave visibly elastic rope segments (max %.3f cm)"), MaximumStretch),
		MaximumStretch < 1.0f);

	const TArray<FVector> BeforeSplit = Rope->GetParticleLocations();
	const FVector WrapPoint(300.0f, 40.0f, 90.0f);
	Rope->SetRoutePath(
		TArray<FVector>{Start, WrapPoint, FVector(590.0f, 0.0f, 100.0f)},
		TArray<int32>{0, 17, -1},
		650.0f);
	TestEqual(TEXT("An authoritative bend divides the visual simulation into two spans"), Rope->GetSpanCount(), 2);
	const TArray<FVector>& AfterSplit = Rope->GetParticleLocations();
	TestTrue(TEXT("Route topology changes preserve a populated visual rope"), AfterSplit.Num() >= BeforeSplit.Num());
	TestTrue(TEXT("The split route remains pinned to its machine endpoint"), AfterSplit[0].Equals(Start, 0.01f));
	TestTrue(TEXT("The split route remains pinned to its player endpoint"),
		AfterSplit.Last().Equals(FVector(590.0f, 0.0f, 100.0f), 0.01f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItTetherPolylineLengthTest,
	"ChopIt.Chain.AuthoritativePathLength",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItTetherPolylineLengthTest::RunTest(const FString& Parameters)
{
	const TArray<FVector> Route{
		FVector(0.0f, 0.0f, 0.0f),
		FVector(300.0f, 0.0f, 0.0f),
		FVector(300.0f, 400.0f, 0.0f),
		FVector(300.0f, 400.0f, 120.0f)};
	TestEqual(
		TEXT("Route length is the exact sum of every independent span"),
		UChopItTetherPathComponent::CalculatePolylineLength(Route),
		820.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItTetherAnchorLifecycleTest,
	"ChopIt.Chain.AnchorLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItTetherAnchorLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	TestNotNull(TEXT("Automation map exists"), World);
	if (!World)
	{
		return false;
	}

	AActor* Owner = World->SpawnActor<AActor>();
	UChopItTetherPathComponent* Path = NewObject<UChopItTetherPathComponent>(Owner);
	Owner->AddInstanceComponent(Path);
	Path->RegisterComponent();
	UChopItChainDefinition* Definition = NewObject<UChopItChainDefinition>();
	Definition->WrapSweepRadius = 5.0f;
	Definition->WrapAnchorSurfaceOffset = 1.0f;
	Definition->WrapMinimumAnchorSeparation = 8.0f;
	Definition->MaximumWrapAnchors = 8;
	Definition->MaximumWrapInsertionsPerFrame = 2;
	Definition->UnwrapConfirmationFrames = 2;
	Path->Configure(Definition);

	AActor* Obstacle = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Obstacle);
	Obstacle->SetRootComponent(Box);
	Obstacle->AddInstanceComponent(Box);
	Box->SetBoxExtent(FVector(50.0f));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Box->SetCollisionResponseToAllChannels(ECR_Block);
	Box->RegisterComponent();
	Obstacle->SetActorLocation(FVector(500.0f, 0.0f, 100.0f));

	const FVector Start(0.0f, 0.0f, 100.0f);
	const FVector End(1000.0f, 0.0f, 100.0f);
	Path->InitializePath(Start, End, nullptr);
	TestTrue(TEXT("A blocking obstacle inserts an ordered route anchor"), Path->GetAnchorCount() >= 1);
	TestTrue(TEXT("The route keeps machine and player as its ordered endpoints"),
		Path->GetRoutePoints()[0].Equals(Start) && Path->GetRoutePoints().Last().Equals(End));

	const FVector AnchorBeforeMove = Path->GetAnchors()[0].GetWorldPosition();
	Obstacle->AddActorWorldOffset(FVector(0.0f, 120.0f, 0.0f));
	const FVector AnchorAfterMove = Path->GetAnchors()[0].GetWorldPosition();
	TestTrue(TEXT("An anchor stored in component space follows a moving prop"),
		AnchorAfterMove.Equals(AnchorBeforeMove + FVector(0.0f, 120.0f, 0.0f), 0.1f));

	Obstacle->SetActorLocation(FVector(500.0f, 1000.0f, 100.0f));
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	for (int32 Frame = 0; Frame < 20; ++Frame)
	{
		Path->UpdatePath(Start, End);
	}
	TestEqual(TEXT("A clean neighbour line removes confirmed obsolete anchors"), Path->GetAnchorCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItTetherMultipleSpanTest,
	"ChopIt.Chain.MultipleIndependentSpans",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItTetherMultipleSpanTest::RunTest(const FString& Parameters)
{
	UChopItChainDefinition* Definition = NewObject<UChopItChainDefinition>();
	Definition->ChainLinkCount = 48;
	Definition->MaxChainLength = 1600.0f;
	Definition->bCableWorldCollision = false;
	Definition->CableGravityScale = 0.0f;

	for (const int32 AnchorCount : TArray<int32>{0, 1, 2, 3, 5})
	{
		TArray<FVector> Route;
		TArray<int32> Ids;
		Route.Add(FVector::ZeroVector);
		Ids.Add(0);
		for (int32 Index = 0; Index < AnchorCount; ++Index)
		{
			Route.Add(FVector(180.0f * (Index + 1), (Index & 1) ? -90.0f : 90.0f, 100.0f));
			Ids.Add(Index + 1);
		}
		Route.Add(FVector(180.0f * (AnchorCount + 1), 0.0f, 100.0f));
		Ids.Add(-1);
		const float RestLength = UChopItTetherPathComponent::CalculatePolylineLength(Route) + 80.0f;

		UChopItRopeComponent* Rope = NewObject<UChopItRopeComponent>();
		Rope->Configure(Definition);
		Rope->InitializeRope(Route[0], Route.Last(), RestLength, nullptr);
		Rope->SetRoutePath(Route, Ids, RestLength);
		for (int32 Frame = 0; Frame < 20; ++Frame)
		{
			Rope->Simulate(1.0f / 60.0f);
		}
		TestEqual(
			*FString::Printf(TEXT("%d anchors create independent spans"), AnchorCount),
			Rope->GetSpanCount(),
			AnchorCount + 1);
		TestTrue(
			*FString::Printf(TEXT("%d anchors cannot accumulate global stretch (%.2f / %.2f cm)"),
				AnchorCount, Rope->GetSimulatedPathLength(), RestLength),
			Rope->GetSimulatedPathLength() <= RestLength + 2.0f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItRopeFixedStepFrameRateTest,
	"ChopIt.Chain.FixedStepAt30_60_120FPS",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItRopeFixedStepFrameRateTest::RunTest(const FString& Parameters)
{
	const auto SimulateAt = [](const int32 FramesPerSecond)
	{
		UChopItChainDefinition* Definition = NewObject<UChopItChainDefinition>();
		Definition->bCableWorldCollision = false;
		UChopItRopeComponent* Rope = NewObject<UChopItRopeComponent>();
		Rope->Configure(Definition);
		Rope->InitializeRope(FVector(0, 0, 200), FVector(700, 0, 200), 850.0f, nullptr);
		for (int32 Frame = 0; Frame < FramesPerSecond; ++Frame)
		{
			Rope->Simulate(1.0f / static_cast<float>(FramesPerSecond));
		}
		const TArray<FVector>& Points = Rope->GetParticleLocations();
		return Points[Points.Num() / 2];
	};
	const FVector At30 = SimulateAt(30);
	const FVector At60 = SimulateAt(60);
	const FVector At120 = SimulateAt(120);
	TestTrue(TEXT("30 and 60 FPS produce the same fixed-step rope"), At30.Equals(At60, 0.5f));
	TestTrue(TEXT("60 and 120 FPS produce the same fixed-step rope"), At60.Equals(At120, 0.5f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChopItTetherDirectionalLimitTest,
	"ChopIt.Chain.DirectionalPlayerLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItTetherDirectionalLimitTest::RunTest(const FString& Parameters)
{
	AActor* Actor = NewObject<AActor>();
	UBoxComponent* Root = NewObject<UBoxComponent>(Actor);
	Actor->SetRootComponent(Root);
	Root->SetWorldLocation(FVector(100.0f, 0.0f, 0.0f));
	UChopItTetherReceiverComponent* Receiver = NewObject<UChopItTetherReceiverComponent>(Actor);
	Receiver->SetTetherState(FVector::ZeroVector, 1.0f, true, 1800.0f, 8.0f);
	TestTrue(TEXT("Hard limit removes only outward input"),
		Receiver->ConstrainMovementDirection(FVector::ForwardVector).IsNearlyZero());
	TestTrue(TEXT("Hard limit always permits return input"),
		Receiver->ConstrainMovementDirection(-FVector::ForwardVector).Equals(-FVector::ForwardVector));
	TestTrue(TEXT("Hard limit always permits tangential input"),
		Receiver->ConstrainMovementDirection(FVector::RightVector).Equals(FVector::RightVector));
	return true;
}
