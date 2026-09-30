#include "Misc/AutomationTest.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"
#include "ChopItCollision.h"
#include "ChopItDeveloperSettings.h"
#include "Combat/ChopItHealthComponent.h"
#include "Harvest/ChopItTree.h"
#include "Feedback/ChopItHitFeedbackComponent.h"
#include "TimerManager.h"
#include "Economy/ChopItChainDefinition.h"
#include "Economy/ChopItDeathRemains.h"
#include "Economy/ChopItQuotaMachine.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Character.h"

#include "GameFramework/WorldSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Economy/ChopItRopeComponent.h"
#include "Economy/ChopItTetherPathComponent.h"
#include "Economy/ChopItTetherReceiverComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "PBDRigidsSolver.h"
#include "Tests/AutomationEditorCommon.h"

namespace ChainV2Tests
{
	struct FScene
	{
		UWorld* World = nullptr;
		bool bGameWorld = false;
		UChopItChainDefinition* Definition = nullptr;
		UChopItRopeComponent* Rope = nullptr;
		explicit FScene(bool bGame = false) : bGameWorld(bGame)
		{
			if (bGameWorld)
			{
				const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true);
				World = UWorld::CreateWorld(EWorldType::GamePreview, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
				GEngine->CreateNewWorldContext(EWorldType::GamePreview).SetCurrentWorld(World);
				World->InitializeActorsForPlay(FURL());
				World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
			}
			else World = FAutomationEditorCommonUtils::CreateNewMap();
			// CreateNewMap runs GC. Allocate transient fixture objects afterwards.
			Definition = NewObject<UChopItChainDefinition>(World);
			Definition->MaxChainLength = 2400;
			Definition->ChainSlack = 40;
			Definition->CableGravityScale = 0;
			Definition->CableCollisionSkin = 0.5f;
			Definition->ChainFeedAcceleration = 8000;
			AActor* Owner = World->SpawnActor<AActor>();
			Rope = NewObject<UChopItRopeComponent>(Owner);
			Owner->AddInstanceComponent(Rope);
			Rope->RegisterComponent();
			Rope->Configure(Definition);
		}
		~FScene()
		{
			if (bGameWorld) { World->BeginTearingDown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
		}
		UBoxComponent* Box(FVector Position, FVector Extent, ECollisionChannel Channel = ECC_WorldStatic)
		{
			AActor* Actor = World->SpawnActor<AActor>();
			UBoxComponent* Shape = NewObject<UBoxComponent>(Actor);
			Actor->SetRootComponent(Shape);
			Actor->AddInstanceComponent(Shape);
			Shape->SetBoxExtent(Extent);
			Shape->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Shape->SetCollisionObjectType(Channel);
			Shape->SetCollisionResponseToAllChannels(ECR_Block);
			Shape->RegisterComponent();
			Shape->SetWorldLocation(Position);
			return Shape;
		}
		void Post(FVector Position)
		{
			AActor* Actor = World->SpawnActor<AActor>();
			UCapsuleComponent* Shape = NewObject<UCapsuleComponent>(Actor);
			Actor->SetRootComponent(Shape);
			Actor->AddInstanceComponent(Shape);
			Shape->InitCapsuleSize(50, 500);
			Shape->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Shape->SetCollisionResponseToAllChannels(ECR_Block);
			Shape->RegisterComponent();
			Shape->SetWorldLocation(Position);
		}
	};
	float Winding(const TArray<FVector>& Points, FVector Origin = FVector::ZeroVector)
	{
		float Angle = 0;
		for (int32 I = 1; I < Points.Num(); ++I)
		{
			const FVector2D A(Points[I - 1].X - Origin.X, Points[I - 1].Y - Origin.Y), B(Points[I].X - Origin.X, Points[I].Y - Origin.Y);
			Angle += FMath::Atan2(A.X * B.Y - A.Y * B.X, FVector2D::DotProduct(A, B));
		}
		return Angle;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2MaterialTest, "ChopIt.Chain.V2.MaterialAndClock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2MaterialTest::RunTest(const FString&)
{
	TArray<FVector> Results;
	for (int32 FPS : {30, 60, 120})
	{
		UChopItChainDefinition* D = NewObject<UChopItChainDefinition>();
		D->bCableWorldCollision = false;
		D->MaxChainLength = 1000;
		UChopItRopeComponent* R = NewObject<UChopItRopeComponent>();
		R->Configure(D);
		R->InitializeRope(FVector(0,0,200), FVector(500,0,200), 600, nullptr);
		for (int32 I = 0; I < FPS; ++I) R->Simulate(1.0f / FPS);
		TestTrue(TEXT("Gravity makes the chain sag"), R->GetParticleLocations()[R->GetParticleLocations().Num()/2].Z < 199);
		TestTrue(TEXT("Length remains bounded"), R->GetSimulatedPathLength() <= 602);
		Results.Add(R->GetParticleLocations()[R->GetParticleLocations().Num()/2]);
		const FVector End = R->GetAcceptedEndpoint();
		R->SetRopeLength(800);
		for (int32 I = 0; I < FPS * 2; ++I) R->Simulate(1.0f / FPS);
		TestTrue(TEXT("Payout increases physical length"), R->GetRopeLength() > 790);
		TestTrue(TEXT("Payout retains the player attachment"), R->GetAcceptedEndpoint().Equals(End, 0.1f));
		R->SetRopeLength(600);
		for (int32 I = 0; I < FPS * 3; ++I) R->Simulate(1.0f / FPS);
		TestTrue(TEXT("Retraction removes length at the outlet"), R->GetRopeLength() < 610);
	}
	TestTrue(TEXT("30 and 60 FPS agree"), Results[0].Equals(Results[1], 0.5f));
	TestTrue(TEXT("60 and 120 FPS agree"), Results[1].Equals(Results[2], 0.5f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2WindingTest, "ChopIt.Chain.V2.WrapAndUnwrap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2WindingTest::RunTest(const FString&)
{
	for (int32 Case : {0, 1, 2})
	{
		const int32 Sign = Case == 0 ? -1 : 1;
		const int32 FPS = Case == 0 ? 30 : (Case == 1 ? 60 : 120);
		ChainV2Tests::FScene Scene;
		Scene.Post(FVector::ZeroVector);
		const FVector Start(-350,0,100);
		Scene.Rope->InitializeRope(Start, FVector(-130,0,100), 270, nullptr);
		Scene.Rope->SetAutomaticReel(true);
		TestTrue(TEXT("Clear initial deployment"), Scene.Rope->IsInitialized());
		float WorstEndError = 0, WorstStretch = 0, PeakTime = 0;
		bool bClear = true;
		const int32 Frames = FPS * 16;
		TArray<float> Timings;
		for (int32 Frame = 1; Frame <= Frames; ++Frame)
		{
			const float Angle = PI + Sign * 4 * PI * Frame / Frames;
			const FVector End(130 * FMath::Cos(Angle), 130 * FMath::Sin(Angle), 100);
			Scene.Rope->SetEndpoints(Start, End);
			Scene.Rope->Simulate(1.0f / FPS);
			WorstEndError = FMath::Max(WorstEndError, static_cast<float>(FVector::Distance(End, Scene.Rope->GetAcceptedEndpoint())));
			WorstStretch = FMath::Max(WorstStretch, Scene.Rope->GetMaximumLengthError());
			PeakTime = FMath::Max(PeakTime, Scene.Rope->GetLastSimulationMilliseconds());
			Timings.Add(Scene.Rope->GetLastSimulationMilliseconds());
			bClear &= Scene.Rope->IsCollisionFree(0.5f);
		}
		Timings.Sort();
		AddInfo(FString::Printf(TEXT("%d FPS: simulation median %.3f ms, P95 %.3f ms"), FPS, Timings[Timings.Num()/2], Timings[FMath::FloorToInt(Timings.Num()*0.95f)]));
		const float WrappedLength = Scene.Rope->GetRopeLength();
		AddInfo(FString::Printf(TEXT("direction %d: winding %.2f, endpoint error %.2f cm, extension %.3f cm, length %.1f, peak %.2f ms, rejected %d"),
			Sign, ChainV2Tests::Winding(Scene.Rope->GetParticleLocations()), WorstEndError, WorstStretch, WrappedLength, PeakTime, Scene.Rope->GetRejectedStepCount()));
		TestTrue(TEXT("Every complete segment stays outside the post"), bClear);
		TestTrue(TEXT("The player actually completes both turns"), WorstEndError < 5);
		TestTrue(TEXT("Both turns are preserved, not shortcut"), Sign * ChainV2Tests::Winding(Scene.Rope->GetParticleLocations()) > 3.5f * PI);
	for (int32 Frame = Frames - 1; Frame >= 0; --Frame)
		{
			const float Angle = PI + Sign * 4 * PI * Frame / Frames;
			Scene.Rope->SetEndpoints(Start, FVector(130 * FMath::Cos(Angle), 130 * FMath::Sin(Angle), 100));
			Scene.Rope->Simulate(1.0f / FPS);
			bClear &= Scene.Rope->IsCollisionFree(0.5f);
		}
		TestTrue(TEXT("Unwrapping never crosses the post"), bClear);
	AddInfo(FString::Printf(TEXT("Returned endpoint %s"), *Scene.Rope->GetAcceptedEndpoint().ToString()));
		TestTrue(TEXT("Reverse travel unwinds both turns"), FMath::Abs(ChainV2Tests::Winding(Scene.Rope->GetParticleLocations())) < 0.25f);
		for (int32 I = 0; I < FPS * 4; ++I) Scene.Rope->Simulate(1.0f/FPS);
		AddInfo(FString::Printf(TEXT("After take-up: length %.2f, tension %.2f, winding %.3f"), Scene.Rope->GetRopeLength(), Scene.Rope->GetEndpointTension(), ChainV2Tests::Winding(Scene.Rope->GetParticleLocations())));
		AddInfo(Scene.Rope->DescribeState());
		TestTrue(TEXT("The reel recovers returned chain"), Scene.Rope->GetRopeLength() <= 280);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2CollisionTest, "ChopIt.Chain.V2.FullSegmentsAndEnemyFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2CollisionTest::RunTest(const FString&)
{
	ChainV2Tests::FScene Scene;
	Scene.Box(FVector(0,0,100), FVector(1,100,200));
	Scene.Rope->InitializeRope(FVector(-100,0,100), FVector(100,0,100), 250, nullptr);
	TestFalse(TEXT("Initialization refuses a thin wall through the segment interior"), Scene.Rope->IsInitialized());
	Scene.Rope->InitializeRope(FVector(-100,-150,100), FVector(100,-150,100), 250, nullptr);
	TestTrue(TEXT("Safe deployment is allowed"), Scene.Rope->IsInitialized());
	// A large single-frame request must stop, not teleport the edge over the wall.
	Scene.Rope->SetEndpoints(FVector(-100,150,100), FVector(100,150,100));
	Scene.Rope->Simulate(0.25f);
	TestTrue(TEXT("Slow frame cannot sweep the chain through a thin wall"), Scene.Rope->IsCollisionFree());
	TestTrue(TEXT("An unsafe endpoint request is rejected"), Scene.Rope->IsMovementBlocked());
	ChainV2Tests::FScene EnemyScene;
	EnemyScene.Box(FVector(0,0,100), FVector(50), ChopItCollisionChannels::Enemy);
	EnemyScene.Rope->InitializeRope(FVector(-100,0,100), FVector(100,0,100), 250, nullptr);
	TestTrue(TEXT("Enemy channel is ignored even when its response says block"), EnemyScene.Rope->IsInitialized());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2LimitTest, "ChopIt.Chain.V2.LengthLimitAndAdapter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2LimitTest::RunTest(const FString&)
{
	UChopItChainDefinition* D = NewObject<UChopItChainDefinition>();
	D->bCableWorldCollision = false;
	D->CableGravityScale = 0;
	D->MaxChainLength = 500;
	UChopItRopeComponent* R = NewObject<UChopItRopeComponent>();
	R->Configure(D);
	R->InitializeRope(FVector::ZeroVector, FVector(490,0,0), 500, nullptr);
	R->SetAutomaticReel(true);
	for (int32 I = 0; I < 60; ++I) { R->SetEndpoints(FVector::ZeroVector, FVector(600,0,0)); R->Simulate(1.0f/60); }
	TestTrue(TEXT("Physical path cannot exceed max plus numerical tolerance"), R->GetSimulatedPathLength() <= 502);
	TestTrue(TEXT("Stored length cannot become negative"), R->GetStoredLength() >= 0);
	TestTrue(TEXT("Outward travel is stopped"), R->IsMovementBlocked());
	for (int32 I = 0; I < 120; ++I) { R->SetEndpoints(FVector::ZeroVector, FVector(350,40,80)); R->Simulate(1.0f/60); }
	AddInfo(FString::Printf(TEXT("Return endpoint %s, length %.2f path %.2f, rejected %d"), *R->GetAcceptedEndpoint().ToString(), R->GetRopeLength(), R->GetSimulatedPathLength(), R->GetRejectedStepCount()));
	TestTrue(TEXT("Return and vertical movement remain possible"), R->GetAcceptedEndpoint().Equals(FVector(350,40,80), 1));
	UChopItTetherPathComponent* Path = NewObject<UChopItTetherPathComponent>();
	Path->SetSource(R);
	TestTrue(TEXT("Adapter contains precisely the physical positions"), Path->GetRoutePoints() == R->GetParticleLocations());
	TestEqual(TEXT("Adapter reports the physical path length"), Path->GetRouteLength(), R->GetSimulatedPathLength());
	const float OldMax = D->MaxChainLength;
	D->MigrateToV2(); D->MigrateToV2();
	TestEqual(TEXT("Migration is versioned and idempotent"), D->PhysicsVersion, 2);
	TestEqual(TEXT("Migration preserves the authored maximum"), D->MaxChainLength, OldMax);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2FloorTest, "ChopIt.Chain.V2.FloorAndMovingObstacle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2FloorTest::RunTest(const FString&)
{
	ChainV2Tests::FScene Scene;
	Scene.Box(FVector(0,0,-25), FVector(800,800,25));
	Scene.Definition->CableGravityScale = 1;
	Scene.Rope->Configure(Scene.Definition);
	Scene.Rope->InitializeRope(FVector(-200,0,100), FVector(200,0,100), 550, nullptr);
	bool bClear = true;
	for (int32 I = 0; I < 240; ++I)
	{
		Scene.Rope->Simulate(1.0f/60);
		bClear &= Scene.Rope->IsCollisionFree(0.5f);
	}
	TestTrue(TEXT("Slack chain rests above the floor for every step"), bClear);
	float Lowest = 100;
	for (const FVector& P : Scene.Rope->GetParticleLocations()) Lowest = FMath::Min(Lowest, static_cast<float>(P.Z));
	TestTrue(TEXT("Gravity actually brings slack to the ground"), Lowest < 10);
	TestTrue(TEXT("Floor contact respects physical thickness"), Lowest >= Scene.Rope->GetCollisionRadius() - 0.5f);

	ChainV2Tests::FScene Moving;
	UBoxComponent* Box = Moving.Box(FVector(0,0,100), FVector(40));
	Moving.Rope->InitializeRope(FVector(-200,-100,100), FVector(200,-100,100), 500, nullptr);
	Box->SetWorldLocation(FVector(0,-70,100));
	for (int32 I = 0; I < 60; ++I) Moving.Rope->Simulate(1.0f/60);
	TestTrue(TEXT("A moving obstacle pushes the complete chain out of penetration"), Moving.Rope->IsCollisionFree(0.5f));
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	for (int32 I = 0; I < 5; ++I) Moving.Rope->Simulate(1.0f/60);
	TestEqual(TEXT("Disabled obstacles leave no persistent pinned contacts"), Moving.Rope->GetContacts().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2MassTest, "ChopIt.Chain.V2.MassAndForces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2MassTest::RunTest(const FString&)
{
	TArray<FVector> Positions;
	for (float Density : {1.0f, 10.0f})
	{
		UChopItChainDefinition* D = NewObject<UChopItChainDefinition>();
		D->bCableWorldCollision = false;
		D->LinearMassDensity = Density;
		UChopItRopeComponent* Rope = NewObject<UChopItRopeComponent>();
		Rope->Configure(D);
		Rope->InitializeRope(FVector(0,0,300), FVector(400,0,300), 700, nullptr);
		for (int32 I = 0; I < 12; ++I) Rope->Simulate(1.0f/120);
		Positions.Add(Rope->GetParticleLocations()[Rope->GetParticleLocations().Num()/2]);
	}
	TestTrue(TEXT("Free fall acceleration is independent of chain mass"), Positions[0].Equals(Positions[1], 0.1f));
	ChainV2Tests::FScene Scene(true);
	UBoxComponent* Body = Scene.Box(FVector(0,0,100), FVector(40), ECC_PhysicsBody);
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetSimulatePhysics(true);
	Body->SetEnableGravity(false);
	Body->SetMassOverrideInKg(NAME_None, 10, true);
	Scene.Rope->InitializeRope(FVector(-300,0,100), FVector(-100,0,100), 250, nullptr);
	Scene.Rope->SetAutomaticReel(true);
	FVector AppliedForce = FVector::ZeroVector;
	float PeakForce = 0;
	for (int32 I = 1; I <= 240; ++I)
	{
		const float Angle = PI + PI * I / 240;
		Scene.Rope->SetEndpoints(FVector(-300,0,100), FVector(100*FMath::Cos(Angle),100*FMath::Sin(Angle),100));
		Scene.Rope->Simulate(1.0f/60);
		for (const FChopItRopeBodyLoad& Load : Scene.Rope->CalculateBodyLoads())
		{
			PeakForce = FMath::Max(PeakForce, static_cast<float>(Load.Force.Size()));
			if (Load.Force.Size() > AppliedForce.Size()) AppliedForce = Load.Force;
		}
	}
	AddInfo(FString::Printf(TEXT("Peak body force %.2f, contacts %d"), PeakForce, Scene.Rope->GetContacts().Num()));
	TestTrue(TEXT("Wrapping a physical box produces nonzero contact load"), PeakForce > 1);
	TestTrue(TEXT("Aggregate force never exceeds the per-body cap"), PeakForce <= Scene.Definition->MaximumPropTensionForce + 1);
	// Advance the real Chaos scene: AddForce/impulses are deferred, so querying
	// velocity without a physics step would incorrectly report zero for both.
	const auto StepPhysics = [&]()
	{
		const FVector Gravity = FVector::ZeroVector;
		FPhysScene* Physics = Scene.World->GetPhysicsScene();
		Physics->SetUpForFrame(&Gravity, 0.01f, 0, 0.1f, 0.01f, 1, false);
		Physics->StartFrame(); Physics->WaitPhysScenes(); Physics->EndFrame();
	};
	StepPhysics();
	Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Body->AddForce(AppliedForce);
	StepPhysics();
	const float LightSpeed = Body->GetPhysicsLinearVelocity().Size();
	Body->SetMassOverrideInKg(NAME_None, 200, true);
	Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Body->AddForce(AppliedForce);
	StepPhysics();
	const float HeavySpeed = Body->GetPhysicsLinearVelocity().Size();
	AddInfo(FString::Printf(TEXT("Chaos speeds: light %.3f, heavy %.3f"), LightSpeed, HeavySpeed));
	TestTrue(TEXT("The same chain load accelerates the light prop more"), LightSpeed > HeavySpeed * 10 && HeavySpeed > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2CornersTest, "ChopIt.Chain.V2.CornersAndSlope",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2CornersTest::RunTest(const FString&)
{
	ChainV2Tests::FScene Scene;
	Scene.Box(FVector(0,0,-25), FVector(800,800,25));
	Scene.Box(FVector(0,0,150), FVector(45,45,200));
	Scene.Box(FVector(0,130,150), FVector(45,45,200));
	UBoxComponent* Slope = Scene.Box(FVector(0,-160,30), FVector(190,80,10));
	Slope->SetWorldRotation(FRotator(12,0,0));
	Scene.Definition->CableGravityScale = 1;
	Scene.Rope->Configure(Scene.Definition);
	const FVector Start(-450,50,140);
	Scene.Rope->InitializeRope(Start, FVector(-240,50,140), 310, nullptr);
	Scene.Rope->SetAutomaticReel(true);
	bool bClear = true;
	float WorstEndError = 0;
	for (int32 I = 1; I <= 480; ++I)
	{
		const float A = PI + 2*PI*I/480;
		const FVector End(240*FMath::Cos(A),50+240*FMath::Sin(A),140);
		Scene.Rope->SetEndpoints(Start, End);
		Scene.Rope->Simulate(1.0f/60);
		bClear &= Scene.Rope->IsCollisionFree(0.5f);
		WorstEndError = FMath::Max(WorstEndError, static_cast<float>(FVector::Distance(End, Scene.Rope->GetAcceptedEndpoint())));
	}
	AddInfo(FString::Printf(TEXT("Two corners + slope: endpoint error %.2f, %s"), WorstEndError, *Scene.Rope->DescribeState()));
	TestTrue(TEXT("Complete segments remain outside floor, slope and both corner obstacles"), bClear);
	TestTrue(TEXT("The player completes the course without sticking"), WorstEndError < 5);
	TestTrue(TEXT("Both obstacles remain inside the loop"), ChainV2Tests::Winding(Scene.Rope->GetParticleLocations()) > 1.8f * PI
		&& ChainV2Tests::Winding(Scene.Rope->GetParticleLocations(), FVector(0,130,0)) > 1.8f * PI);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2CharacterTest, "ChopIt.Chain.V2.CharacterMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2CharacterTest::RunTest(const FString&)
{
	ChainV2Tests::FScene Scene(true);
	Scene.Box(FVector(0,0,-25), FVector(1500,1500,25));
	Scene.Definition->MaxChainLength = 500;
	Scene.Definition->CableGravityScale = 1;
	ACharacter* Character = Scene.World->SpawnActor<ACharacter>(FVector(300,0,100), FRotator::ZeroRotator);
	UChopItTetherReceiverComponent* Receiver = NewObject<UChopItTetherReceiverComponent>(Character);
	Character->AddInstanceComponent(Receiver);
	Receiver->RegisterComponent();
	APlayerController* Controller = Scene.World->SpawnActor<APlayerController>();
	Controller->SetAsLocalPlayerController();
	Controller->Possess(Character);
	AChopItQuotaMachine* Machine = Scene.World->SpawnActorDeferred<AChopItQuotaMachine>(AChopItQuotaMachine::StaticClass(), FTransform::Identity);
	Machine->SetChainDefinition(Scene.Definition);
	Machine->FinishSpawning(FTransform::Identity);
	if (!Machine->HasActorBegunPlay()) Machine->DispatchBeginPlay();
	UChopItRopeComponent* Rope = Machine->FindComponentByClass<UChopItRopeComponent>();
	TestTrue(TEXT("The machine binds its player at BeginPlay"), Rope && Rope->IsInitialized());
	if (!Rope || !Rope->IsInitialized()) return false;
	float WorstAttachmentError = 0, Farthest = 0, Highest = 0;
	bool bClear = true;
	for (int32 I=0; I<300; ++I)
	{
		Character->AddMovementInput(I < 180 ? FVector::ForwardVector : FVector(0,1,0));
		if (I == 60) Character->Jump();
		if (I == 90) Character->StopJumping();
		// CharacterMovement guards against running twice in one engine frame.
		++GFrameCounter;
		Scene.World->Tick(LEVELTICK_All, I == 150 ? 0.15f : 1.0f/60);
		const FVector Anchor = Character->GetActorTransform().TransformPosition(Scene.Definition->PlayerChainAnchor);
		WorstAttachmentError = FMath::Max(WorstAttachmentError, static_cast<float>(FVector::Distance(Anchor, Rope->GetAcceptedEndpoint())));
		Farthest = FMath::Max(Farthest, static_cast<float>(Character->GetActorLocation().X));
		Highest = FMath::Max(Highest, static_cast<float>(Character->GetActorLocation().Z));
		bClear &= Rope->IsCollisionFree(0.5f);
	}
	AddInfo(FString::Printf(TEXT("Character: attachment %.3f cm, furthest %.1f, height %.1f, final %s"), WorstAttachmentError, Farthest, Highest, *Character->GetActorLocation().ToString()));
	TestTrue(TEXT("Character and rope endpoints agree after world ticks"), WorstAttachmentError < 1);
	TestTrue(TEXT("Character reaches the maximum without escaping it"), Farthest > 420 && Farthest < 505);
	TestTrue(TEXT("The character can jump while tethered"), Highest > 150);
	TestTrue(TEXT("The maximum still permits lateral travel"), Character->GetActorLocation().Y > 100);
	TestTrue(TEXT("Character movement and slow frames preserve full segment clearance"), bClear);
	TestTrue(TEXT("Physical deployed length respects the configured maximum"), Rope->GetRopeLength() <= 500.01f);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2WrappedLimitTest, "ChopIt.Chain.V2.WrappedLengthLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2WrappedLimitTest::RunTest(const FString&)
{
	ChainV2Tests::FScene Scene;
	Scene.Definition->MaxChainLength = 900;
	Scene.Rope->Configure(Scene.Definition);
	Scene.Post(FVector::ZeroVector);
	const FVector Start(-350,0,100);
	Scene.Rope->InitializeRope(Start, FVector(-130,0,100), 270, nullptr);
	Scene.Rope->SetAutomaticReel(true);
	bool bClear = true;
	for (int32 I=1; I<=600; ++I)
	{
		const float A = PI + 2*PI*I/600;
		Scene.Rope->SetEndpoints(Start, FVector(130*FMath::Cos(A),130*FMath::Sin(A),100));
		Scene.Rope->Simulate(1.0f/60);
		bClear &= Scene.Rope->IsCollisionFree(0.5f);
	}
	TestTrue(TEXT("The course actually puts one complete turn around the post"), ChainV2Tests::Winding(Scene.Rope->GetParticleLocations()) > 1.8f*PI);
	for (int32 I=1; I<=300; ++I)
	{
		Scene.Rope->SetEndpoints(Start, FVector(-130-I*3,0,100));
		Scene.Rope->Simulate(1.0f/60);
		bClear &= Scene.Rope->IsCollisionFree(0.5f);
	}
	TestTrue(TEXT("The wrapped chain consumes the remaining spool"), Scene.Rope->GetStoredLength() < 0.1f);
	TestTrue(TEXT("The maximum blocks extension while preserving the turn"), Scene.Rope->IsMovementBlocked() && ChainV2Tests::Winding(Scene.Rope->GetParticleLocations()) > 1.8f*PI);
	TestTrue(TEXT("Wrapped physical length stays within the unchanged tolerance"), Scene.Rope->GetSimulatedPathLength() <= 902);
	const FVector AtLimit = Scene.Rope->GetAcceptedEndpoint();
	const FVector Lateral = AtLimit + FVector(0,15,0);
	for (int32 I=0; I<60; ++I) { Scene.Rope->SetEndpoints(Start, Lateral); Scene.Rope->Simulate(1.0f/60); bClear &= Scene.Rope->IsCollisionFree(0.5f); }
	TestTrue(TEXT("A lateral step remains possible with a wrapped chain at maximum"), Scene.Rope->GetAcceptedEndpoint().Equals(Lateral, 1));
	const FVector Return = Lateral + FVector(50,0,0);
	for (int32 I=0; I<60; ++I) { Scene.Rope->SetEndpoints(Start, Return); Scene.Rope->Simulate(1.0f/60); bClear &= Scene.Rope->IsCollisionFree(0.5f); }
	AddInfo(FString::Printf(TEXT("Wrapped return error %.3f cm: %s"), FVector::Distance(Scene.Rope->GetAcceptedEndpoint(), Return), *Scene.Rope->DescribeState()));
	TestTrue(TEXT("Returning from a wrapped maximum remains possible"), Scene.Rope->GetAcceptedEndpoint().Equals(Return, 1));
	TestTrue(TEXT("Every complete segment remains clear throughout the wrapped limit test"), bClear);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2FrameBudgetTest, "ChopIt.Chain.V2.FrameBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2FrameBudgetTest::RunTest(const FString&)
{
	IConsoleVariable* Budget = IConsoleManager::Get().FindConsoleVariable(TEXT("ChopIt.Chain.FrameBudgetMs"));
	if (!TestNotNull(TEXT("Runtime simulation budget is registered"), Budget)) return false;
	const float Original = Budget->GetFloat();
	ON_SCOPE_EXIT { Budget->Set(Original, ECVF_SetByConsole); };
	ChainV2Tests::FScene Scene;
	Scene.Post(FVector::ZeroVector);
	const FVector Start(-350, 0, 100), End(-130, 0, 100);
	Scene.Rope->InitializeRope(Start, End, 270, nullptr);
	const TArray<FVector> Before = Scene.Rope->GetParticleLocations();
	Budget->Set(0.000001f, ECVF_SetByConsole);
	Scene.Rope->SetEndpoints(Start, FVector(200, 0, 100));
	Scene.Rope->Simulate(0.5f);
	TestTrue(TEXT("Exhausted budget retains the complete shape instead of shortcutting the post"), Before == Scene.Rope->GetParticleLocations());
	TestTrue(TEXT("Budget exhaustion blocks incompatible endpoint travel"), Scene.Rope->IsMovementBlocked());
	TestTrue(TEXT("Budget exit still records CPU time"), Scene.Rope->GetLastSimulationMilliseconds() > 0);
	Budget->Set(6.0f, ECVF_SetByConsole);
	Scene.Rope->SetEndpoints(Start, End + FVector(0, 1, 0));
	Scene.Rope->Simulate(1.0f / 120.0f);
	TestTrue(TEXT("Simulation resumes after overload without replaying half a second of debt"), Scene.Rope->GetAcceptedEndpoint().Equals(End + FVector(0, 1, 0), 0.1));
	TestTrue(TEXT("Recovered full segments remain clear"), Scene.Rope->IsCollisionFree());
	// Long chain on the floor, abrupt direction reversals and slow frames:
	// exercise the actual contact/retry workload, not only the forced early exit.
	Budget->Set(12.0f, ECVF_SetByConsole);
	Scene.Box(FVector(-1000,0,-25), FVector(2000,2000,25));
	Scene.Definition->CableGravityScale = 1;
	Scene.Rope->Configure(Scene.Definition);
	Scene.Rope->InitializeRope(FVector(-2100,0,8), FVector(-100,0,8), 2100, nullptr);
	float PeakMs = 0;
	bool bClear = true;
	for (int32 Frame = 0; Frame < 90; ++Frame)
	{
		Scene.Rope->SetEndpoints(FVector(-2100,0,8), FVector(300, Frame % 2 ? 600 : -600, 8));
		Scene.Rope->Simulate(Frame % 10 == 0 ? 0.25f : 1.0f / 60.0f);
		PeakMs = FMath::Max(PeakMs, Scene.Rope->GetLastSimulationMilliseconds());
		bClear &= Scene.Rope->IsCollisionFree(0.5f);
	}
	AddInfo(FString::Printf(TEXT("Contact overload: peak %.3f ms, %s"), PeakMs, *Scene.Rope->DescribeState()));
	TestTrue(TEXT("Overload preserves complete segment clearance"), bClear);
	TestTrue(TEXT("Contact retries do not monopolize a 100 ms frame"), PeakMs < 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2RoundTripTest, "ChopIt.Chain.V2.CharacterRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2RoundTripTest::RunTest(const FString&)
{
	ChainV2Tests::FScene Scene(true);
	Scene.Box(FVector(0,0,-25), FVector(1500,1500,25));
	Scene.Definition->MaxChainLength = 3600;
	Scene.Definition->CableGravityScale = 1;
	Scene.Definition->ChainSlack = 200;
	ACharacter* Character = Scene.World->SpawnActor<ACharacter>(FVector(300,0,100), FRotator::ZeroRotator);
	UChopItTetherReceiverComponent* Receiver = NewObject<UChopItTetherReceiverComponent>(Character);
	Character->AddInstanceComponent(Receiver);
	Receiver->RegisterComponent();
	APlayerController* Controller = Scene.World->SpawnActor<APlayerController>();
	Controller->SetAsLocalPlayerController();
	Controller->Possess(Character);
	AChopItQuotaMachine* Machine = Scene.World->SpawnActorDeferred<AChopItQuotaMachine>(AChopItQuotaMachine::StaticClass(), FTransform::Identity);
	Machine->SetChainDefinition(Scene.Definition);
	Machine->FinishSpawning(FTransform::Identity);
	if (!Machine->HasActorBegunPlay()) Machine->DispatchBeginPlay();
	UChopItRopeComponent* Rope = Machine->FindComponentByClass<UChopItRopeComponent>();
	TestTrue(TEXT("The machine binds its player at BeginPlay"), Rope && Rope->IsInitialized());
	if (!Rope || !Rope->IsInitialized()) return false;

    Character->GetCharacterMovement()->MaxWalkSpeed = 300;
    float OutX = 0;
    float OutLength = 0;
    float FirstReturnLength = 0;
    int32 Stalls = 0;
    bool bClear = true;
    bool bAnchored = true;
    for (int32 I = 0; I < 1080; ++I)
    {
        const int32 Phase = I % 360;
        const FVector Before = Character->GetActorLocation();
        Character->AddMovementInput(Phase < 180 ? FVector::ForwardVector : -FVector::ForwardVector);
        ++GFrameCounter;
        Scene.World->Tick(LEVELTICK_All, 1.0f/60);
        if (Phase == 179) { OutX = Character->GetActorLocation().X; OutLength = Rope->GetRopeLength(); }
        if (Phase > 190 && Before.X - Character->GetActorLocation().X < 1) ++Stalls;
        bClear &= Rope->IsCollisionFree(0.5f);
        bAnchored &= Rope->GetAcceptedEndpoint().Equals(
            Character->GetActorTransform().TransformPosition(Scene.Definition->PlayerChainAnchor), 0.01f);
        if (Phase == 359)
        {
            if (I == 359) FirstReturnLength = Rope->GetRopeLength();
            TestTrue(TEXT("Repeated trips reuse slack instead of accumulating material"), Rope->GetRopeLength() <= FirstReturnLength + 10);
            TestTrue(TEXT("Each outward leg completes"), OutX > 1100);
            TestTrue(TEXT("Each return leg completes without accumulated drag"), Character->GetActorLocation().X < 450);
            TestTrue(TEXT("Each return retrieves material"), Rope->GetRopeLength() < OutLength - 300);
        }
        if (I % 60 == 59)
            AddInfo(FString::Printf(TEXT("Round trip frame %d position %s: %s"), I,
                *Character->GetActorLocation().ToString(), *Rope->DescribeState()));
    }
    TestTrue(TEXT("Player completes outward leg"), OutX > 1100);
    TestTrue(TEXT("Player returns over the same empty floor"), Character->GetActorLocation().X < 450);
    TestTrue(TEXT("Loose chain does not repeatedly arrest return movement"), Stalls < 10);
    TestTrue(TEXT("Returning retrieves material instead of feeding a floor loop"), Rope->GetRopeLength() < OutLength - 300);
    TestTrue(TEXT("Every segment remains clear during return"), bClear);
    TestTrue(TEXT("Reeling preserves the attached endpoint"), bAnchored);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2ProductionReturnTest, "ChopIt.Chain.V2.ProductionReturn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2ProductionReturnTest::RunTest(const FString&)
{
    for (const int32 FPS : {30, 60, 120})
    {
    AddInfo(FString::Printf(TEXT("Production return at %d FPS"), FPS));
    ChainV2Tests::FScene Scene(true);
    Scene.Box(FVector(0,0,-25), FVector(5000,5000,25));
    UChopItChainDefinition* Preset = LoadObject<UChopItChainDefinition>(nullptr,
        TEXT("/Game/ChopIt/World/ChainLab/DA_Chain_Default.DA_Chain_Default"));
    if (!TestNotNull(TEXT("Saved production preset exists"), Preset)) return false;
    ACharacter* Character = Scene.World->SpawnActor<ACharacter>(FVector(300,0,100), FRotator::ZeroRotator);
    Character->GetCapsuleComponent()->InitCapsuleSize(42,88);
    Character->GetCharacterMovement()->MaxWalkSpeed = 650;
    Character->GetCharacterMovement()->BrakingDecelerationWalking = 2200;
    UChopItTetherReceiverComponent* Receiver = NewObject<UChopItTetherReceiverComponent>(Character);
    Character->AddInstanceComponent(Receiver);
    Receiver->RegisterComponent();
    APlayerController* Controller = Scene.World->SpawnActor<APlayerController>();
    Controller->SetAsLocalPlayerController();
    Controller->Possess(Character);
    AChopItQuotaMachine* Machine = Scene.World->SpawnActorDeferred<AChopItQuotaMachine>(AChopItQuotaMachine::StaticClass(), FTransform::Identity);
    Machine->SetChainDefinition(Preset);
    Machine->FinishSpawning(FTransform::Identity);
    if (!Machine->HasActorBegunPlay()) Machine->DispatchBeginPlay();
    UChopItRopeComponent* Rope = Machine->FindComponentByClass<UChopItRopeComponent>();
    if (!TestTrue(TEXT("Production chain initialized"), Rope && Rope->IsInitialized())) return false;
    AddInfo(FString::Printf(TEXT("Production speed %.1f feed %.1f acceleration %.1f slack %.1f max %.1f radius %.1f"),
        Character->GetCharacterMovement()->MaxWalkSpeed, Preset->ChainFeedSpeed, Preset->ChainFeedAcceleration,
        Preset->ChainSlack, Preset->MaxChainLength, Preset->CableParticleDiameter * .5f));
    int32 SlowReturnFrames = 0;
    bool bClear = true;
    for (int32 Frame = 0; Frame < 8 * FPS; ++Frame)
    {
        // Extend for 3 s, return for 3 s, let the reel gather the folded chain for 2 s.
        const FVector Before = Character->GetActorLocation();
        if (Frame < 6 * FPS) Character->AddMovementInput(Frame < 3 * FPS ? FVector::ForwardVector : -FVector::ForwardVector);
        ++GFrameCounter;
        Scene.World->Tick(LEVELTICK_All, 1.0f / FPS);
        if (Frame > 3.5f * FPS && Frame < 5.8f * FPS && Before.X - Character->GetActorLocation().X < 0.8f * 650 / FPS) ++SlowReturnFrames;
        bClear &= Rope->IsCollisionFree(.5f);
        if (Frame % FPS == FPS - 1)
            AddInfo(FString::Printf(TEXT("Production frame %d X %.2f V %.2f: %s"), Frame,
                Character->GetActorLocation().X, Character->GetCharacterMovement()->Velocity.X, *Rope->DescribeState()));
        if (Frame == 3 * FPS - 1) {
            TestTrue(TEXT("Production character completes outward leg"), Character->GetActorLocation().X > 2000);
        }
        if (Frame == 6 * FPS - 1) TestTrue(TEXT("Production character returns without chain drag"), Character->GetActorLocation().X < 600);
    }
    TestTrue(TEXT("Return maintains production walking speed"), SlowReturnFrames < FPS / 6);
    TestTrue(TEXT("Production segments remain clear"), bClear);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2ObstacleWalkTest, "ChopIt.Chain.V2.ProductionObstacleWalk",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2ObstacleWalkTest::RunTest(const FString&)
{
    for (const bool bConvexPost : {false, true})
    {
    for (const int32 FPS : {30, 60, 120})
    {
    AddInfo(FString::Printf(TEXT("Production obstacle walk (%s) at %d FPS"), bConvexPost ? TEXT("map cylinder") : TEXT("capsule"), FPS));
    ChainV2Tests::FScene Scene(true);
    Scene.Box(FVector(0,0,-25), FVector(3000,3000,25));
    if (!bConvexPost) Scene.Post(FVector::ZeroVector);
    else
    {
        AActor* Post = Scene.World->SpawnActor<AActor>();
        auto* Mesh = NewObject<UStaticMeshComponent>(Post);
        Post->SetRootComponent(Mesh);
        Post->AddInstanceComponent(Mesh);
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
        Mesh->SetWorldScale3D(FVector(1.4,1.4,10));
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Mesh->SetCollisionObjectType(ECC_WorldStatic);
        Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        Mesh->SetMobility(EComponentMobility::Static);
        Mesh->RegisterComponent();
    }
    UChopItChainDefinition* Preset = LoadObject<UChopItChainDefinition>(nullptr,
        TEXT("/Game/ChopIt/World/ChainLab/DA_Chain_Default.DA_Chain_Default"));
    if (!TestNotNull(TEXT("Production preset exists"), Preset)) return false;
    ACharacter* Character = Scene.World->SpawnActor<ACharacter>(FVector(-250,-250,100), FRotator::ZeroRotator);
    Character->GetCapsuleComponent()->InitCapsuleSize(42,88);
    Character->GetCharacterMovement()->MaxWalkSpeed = 650;
    Character->GetCharacterMovement()->BrakingDecelerationWalking = 2200;
    auto* Receiver = NewObject<UChopItTetherReceiverComponent>(Character);
    Character->AddInstanceComponent(Receiver);
    Receiver->RegisterComponent();
    auto* Controller = Scene.World->SpawnActor<APlayerController>();
    Controller->SetAsLocalPlayerController();
    Controller->Possess(Character);
    const FTransform MachineTransform(FVector(-600,-250,0));
    auto* Machine = Scene.World->SpawnActorDeferred<AChopItQuotaMachine>(AChopItQuotaMachine::StaticClass(), MachineTransform);
    Machine->SetChainDefinition(Preset);
    Machine->FinishSpawning(MachineTransform);
    if (!Machine->HasActorBegunPlay()) Machine->DispatchBeginPlay();
    auto* Rope = Machine->FindComponentByClass<UChopItRopeComponent>();
    if (!TestTrue(TEXT("Machine binds character"), Rope && Rope->IsInitialized())) return false;
    const FVector Corners[] = {FVector(250,-250,0), FVector(250,250,0), FVector(-250,250,0), FVector(-250,-250,0)};
    int32 Waypoint = 0, SlowFrames = 0, FramesSinceTurn = 0;
    bool bClear = true, bLengthValid = true;
    float WrappedWinding = 0;
    int32 MostContacts = 0;
    TArray<float> Times;
    for (int32 Frame = 0; Frame < 24 * FPS && Waypoint < 16; ++Frame)
    {
        const auto NextCorner = [&]() { return Waypoint < 8 ? Waypoint % 4 : (18 - Waypoint) % 4; };
        FVector Offset = Corners[NextCorner()] - Character->GetActorLocation();
        Offset.Z = 0;
        if (Offset.Size() < 35) {
            ++Waypoint; FramesSinceTurn = 0;
            if (Waypoint == 8) WrappedWinding = ChainV2Tests::Winding(Rope->GetParticleLocations());
            if (Waypoint == 16) break;
            Offset = Corners[NextCorner()] - Character->GetActorLocation(); Offset.Z = 0;
        }
        const FVector Before = Character->GetActorLocation();
        Character->AddMovementInput(Offset.GetSafeNormal());
        ++GFrameCounter;
        Scene.World->Tick(LEVELTICK_All, 1.0f / FPS);
        ++FramesSinceTurn;
        if (FramesSinceTurn > FPS / 3 && Offset.Size() > 100 && FVector::Dist2D(Before, Character->GetActorLocation()) < 480.0f / FPS) ++SlowFrames;
        bClear &= Rope->IsCollisionFree(.5f);
        bLengthValid &= Rope->GetSimulatedPathLength() <= Rope->GetRopeLength() + 2.0f && Rope->GetMaximumLengthError() <= 1.0f;
        MostContacts = FMath::Max(MostContacts, Rope->GetContacts().Num());
        Times.Add(Rope->GetLastSimulationMilliseconds());
        if (Frame % FPS == FPS - 1) AddInfo(FString::Printf(TEXT("Obstacle walk frame %d waypoint %d position %s speed %.1f: %s"), Frame, Waypoint,
            *Character->GetActorLocation().ToString(), Character->GetCharacterMovement()->Velocity.Size2D(), *Rope->DescribeState()));
    }
    Times.Sort();
    const float P95 = Times[FMath::Min(Times.Num()-1, FMath::FloorToInt(Times.Num() * .95f))];
    AddInfo(FString::Printf(TEXT("Obstacle walk: waypoints %d/16 slow frames %d contacts %d P95 %.2f ms stock %.1f winding %.2f"),
        Waypoint, SlowFrames, MostContacts, P95, Rope->GetStoredLength(), ChainV2Tests::Winding(Rope->GetParticleLocations())));
    TestTrue(TEXT("Two complete laps and their reverse remain possible before the material maximum"), Waypoint == 16);
    TestTrue(TEXT("Contact does not repeatedly brake the player while material remains"), SlowFrames < FPS / 2);
    TestTrue(TEXT("The path actually exercised contacts"), MostContacts > 0);
    TestTrue(TEXT("Both physical wraps remain around the post before reversing"), WrappedWinding > 3 * PI);
    TestTrue(TEXT("Returning along the same route removes both wraps"), FMath::Abs(ChainV2Tests::Winding(Rope->GetParticleLocations())) < PI);
    TestTrue(TEXT("All full segments remain outside the post and floor"), bClear);
    TestTrue(TEXT("Length stays inside the unchanged physical limits"), bLengthValid);
    // A 30 FPS frame contains twice as many fixed steps as a 60 FPS frame.
    // Keep the original 16 ms / 60 FPS cost target identical at each rate.
    TestTrue(TEXT("Obstacle work stays within the fixed-step cost budget at P95"), P95 * FPS / 60.0f < 16);
    }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChainV2TreeHitTest, "ChopIt.Chain.V2.TreeHitClearance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChainV2TreeHitTest::RunTest(const FString&)
{
    ChainV2Tests::FScene Scene;
    auto* Settings = GetMutableDefault<UChopItDeveloperSettings>();
    const bool bOriginalFlash = Settings->bEnableHitFlash;
    Settings->bEnableHitFlash = true;
    ON_SCOPE_EXIT { Settings->bEnableHitFlash = bOriginalFlash; };
    auto* Tree = Scene.World->SpawnActor<AChopItTree>(FVector(0,0,380), FRotator::ZeroRotator);
    // Exercise the actual health/feedback event without booting unrelated
    // game-only targeting subsystems in the isolated editor world.
    Tree->GetTrunkMesh()->SetCollisionResponseToChannel(ChopItCollisionChannels::Chain, ECR_Ignore);
    Tree->GetCrownMesh()->SetCollisionResponseToChannel(ChopItCollisionChannels::Chain, ECR_Ignore);
    auto* Feedback = Tree->FindComponentByClass<UChopItHitFeedbackComponent>();
    Feedback->BeginPlay();
    ON_SCOPE_EXIT { Feedback->EndPlay(EEndPlayReason::EndPlayInEditor); };
    const FTransform PhysicalTransform = Tree->GetPhysicsRoot()->GetComponentTransform();
    const FVector VisualScale = Tree->GetTrunkMesh()->GetRelativeScale3D();
    Scene.Rope->InitializeRope(FVector(-150,45,10), FVector(150,45,10), 300, nullptr);
    if (!TestTrue(TEXT("Chain starts clear beside the rounded tree base"), Scene.Rope->IsInitialized())) return false;
    ++GFrameCounter;
    Scene.World->GetTimerManager().Tick(0.0f);
    for (const bool bCritical : {false, true})
    {
        FChopItDamageSpec Damage;
        Damage.BaseDamage = 1;
        Damage.bCritical = bCritical;
        Tree->GetHealthComponent()->ApplyDamage(Damage, nullptr, FVector(0,0,100));
        TestTrue(TEXT("Hit feedback does not transform the physical tree capsule"), Tree->GetPhysicsRoot()->GetComponentTransform().Equals(PhysicalTransform));
        TestTrue(TEXT("The hit still animates the visual trunk"), !Tree->GetTrunkMesh()->GetRelativeScale3D().Equals(VisualScale));
        TestTrue(TEXT("Every segment stays clear during a normal or critical hit"), Scene.Rope->IsCollisionFree(.5f));
        ++GFrameCounter;
        Scene.World->GetTimerManager().Tick(.2f);
        TestTrue(TEXT("Visual pulse restores without moving the physical tree"), Tree->GetTrunkMesh()->GetRelativeScale3D().Equals(VisualScale)
            && Tree->GetPhysicsRoot()->GetComponentTransform().Equals(PhysicalTransform));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeathRemainsPhysicsTest, "ChopIt.Death.RemainsPhysics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDeathRemainsPhysicsTest::RunTest(const FString&)
{
	ChainV2Tests::FScene Scene(true);
	Scene.Box(FVector(0, 0, -25), FVector(1500, 1500, 25));
	AChopItDeathRemains* Remains = Scene.World->SpawnActor<AChopItDeathRemains>(FVector(0, 0, 250), FRotator::ZeroRotator);
	Remains->InitializeRemains(FVector::ForwardVector);
	TInlineComponentArray<UStaticMeshComponent*> Fragments(Remains);
	TestEqual(TEXT("The machine emits blood droplets and meat fragments"), Fragments.Num(), 22);
	bool bAllPhysical = true;
	for (UStaticMeshComponent* Fragment : Fragments)
	{
		bAllPhysical &= Fragment && Fragment->IsSimulatingPhysics() && Fragment->IsCollisionEnabled();
	}
	TestTrue(TEXT("Every emitted fragment has gravity and collision"), bAllPhysical);
	const float InitialHeight = Fragments.IsEmpty() ? 0.0f : Fragments[0]->GetComponentLocation().Z;
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		++GFrameCounter;
		Scene.World->Tick(LEVELTICK_All, 1.0f / 60.0f);
	}
	TestTrue(TEXT("A physical fragment falls instead of fading in place"), !Fragments.IsEmpty()
		&& Fragments[0]->GetComponentLocation().Z < InitialHeight - 10.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeathPullObstacleTest, "ChopIt.Death.PullPastObstacle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDeathPullObstacleTest::RunTest(const FString&)
{
	ChainV2Tests::FScene Scene(true);
	Scene.Box(FVector(0, 0, -25), FVector(1500, 1500, 25));
	// The physical chain clears this gap; the wider player capsule does not.
	Scene.Post(FVector(250, 75, 100));
	Scene.Post(FVector(250, -75, 100));
	Scene.Definition->MaxChainLength = 800.0f;
	Scene.Definition->ChainFeedSpeed = 650.0f;
	Scene.Definition->ChainFeedAcceleration = 3200.0f;
	ACharacter* Character = Scene.World->SpawnActor<ACharacter>(FVector(550, 0, 100), FRotator::ZeroRotator);
	UChopItTetherReceiverComponent* Receiver = NewObject<UChopItTetherReceiverComponent>(Character);
	Character->AddInstanceComponent(Receiver);
	Receiver->RegisterComponent();
	APlayerController* Controller = Scene.World->SpawnActor<APlayerController>();
	Controller->SetAsLocalPlayerController();
	Controller->Possess(Character);
	AChopItQuotaMachine* Machine = Scene.World->SpawnActorDeferred<AChopItQuotaMachine>(
		AChopItQuotaMachine::StaticClass(), FTransform::Identity);
	Machine->SetChainDefinition(Scene.Definition);
	Machine->FinishSpawning(FTransform::Identity);
	if (!Machine->HasActorBegunPlay()) Machine->DispatchBeginPlay();
	UChopItRopeComponent* Rope = Machine->FindComponentByClass<UChopItRopeComponent>();
	TestTrue(TEXT("Death fixture starts with a physical chain"), Rope && Rope->IsInitialized());
	if (!Rope || !Rope->IsInitialized()) return false;
	Character->GetCharacterMovement()->DisableMovement();
	Machine->BeginDeathSequenceForAutomation();
	float MaximumAttachmentError = 0.0f;
	float MinimumX = Character->GetActorLocation().X;
	float ClearPullTravel = 0.0f;
	float ClearReelTravel = 0.0f;
	float PreviousX = Character->GetActorLocation().X;
	float PreviousLength = Rope->GetRopeLength();
	bool bConsumed = false;
	bool bStalled = false;
	for (int32 Frame = 0; Frame < 600; ++Frame)
	{
		++GFrameCounter;
		Scene.World->Tick(LEVELTICK_All, 1.0f / 60.0f);
		MinimumX = FMath::Min(MinimumX, static_cast<float>(Character->GetActorLocation().X));
		if (Machine->IsDeathPresentationReady() && Rope->IsInitialized()) { bStalled = true; break; }
		if (!Rope->IsInitialized()) { bConsumed = !Character->GetActorEnableCollision(); break; }
		const float CurrentX = Character->GetActorLocation().X;
		const float CurrentLength = Rope->GetRopeLength();
		if (Frame >= 2 && PreviousX > 350.0f && CurrentX > 350.0f)
		{
			ClearPullTravel += FMath::Max(0.0f, PreviousX - CurrentX);
			ClearReelTravel += FMath::Max(0.0f, PreviousLength - CurrentLength);
		}
		PreviousX = CurrentX;
		PreviousLength = CurrentLength;
		const FVector Anchor = Character->GetActorTransform().TransformPosition(Scene.Definition->PlayerChainAnchor);
		MaximumAttachmentError = FMath::Max(MaximumAttachmentError,
			static_cast<float>(FVector::Distance(Anchor, Rope->GetAcceptedEndpoint())));
	}
	AddInfo(FString::Printf(TEXT("Death pull: minimum X %.1f, final %s, attachment %.3f cm, clear pull %.1f cm, reel %.1f cm, consumed %d"),
		MinimumX, *Character->GetActorLocation().ToString(), MaximumAttachmentError,
		ClearPullTravel, ClearReelTravel, bConsumed));
	TestTrue(TEXT("The capsule remains blocked by a gap it cannot fit through"), MinimumX > 270.0f);
	TestTrue(TEXT("The chain stays attached during the pull"), MaximumAttachmentError < 1.0f);
	TestTrue(TEXT("An impossible capsule gap ends without repositioning"),
		bStalled && !bConsumed && Machine->GetDeathStallCountForAutomation() > 0);
	TestEqual(TEXT("Temporary pawn collision ignores are restored after the stall"),
		Character->GetCapsuleComponent()->GetMoveIgnoreActors().Num(), 0);
	TestTrue(TEXT("The cinematic reel keeps up with the pull on a clear span"),
		ClearPullTravel > 50.0f && ClearReelTravel >= ClearPullTravel * 0.95f);
	TestTrue(TEXT("Blocked death ends without moving the character into the machine"), bStalled && !bConsumed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeathPullScenariosTest, "ChopIt.Death.ScenariosAtoF",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDeathPullScenariosTest::RunTest(const FString&)
{
	for (int32 Scenario = 0; Scenario < 7; ++Scenario)
	{
		ChainV2Tests::FScene Scene(true);
		Scene.Box(FVector(0, 0, -25), FVector(1500, 1500, 25));
		Scene.Definition->MaxChainLength = Scenario == 6 ? 2400.0f : 900.0f;
		Scene.Definition->ChainFeedSpeed = 650.0f;
		Scene.Definition->ChainFeedAcceleration = 3200.0f;
		Scene.Definition->DeathPullStuckTime = 0.5f;
		if (Scenario == 6)
		{
			UCurveFloat* SpeedCurve = NewObject<UCurveFloat>(Scene.Definition);
			SpeedCurve->FloatCurve.AddKey(0.0f, 0.0f);
			SpeedCurve->FloatCurve.AddKey(2.0f, 1.0f);
			Scene.Definition->DeathRetractionCurve = SpeedCurve;
		}
		if (Scenario == 1) Scene.Post(FVector(250, 70, 100));
		if (Scenario == 2) Scene.Box(FVector(250, 105, 110), FVector(35, 72, 110));
		if (Scenario == 3)
		{
			Scene.Post(FVector(250, 75, 100));
			Scene.Post(FVector(250, -75, 100));
		}
		ACharacter* Character = Scene.World->SpawnActor<ACharacter>(
			FVector(Scenario == 6 ? 2200.0f : 550.0f, 0, 100), FRotator::ZeroRotator);
		UChopItTetherReceiverComponent* Receiver = NewObject<UChopItTetherReceiverComponent>(Character);
		Character->AddInstanceComponent(Receiver);
		Receiver->RegisterComponent();
		APlayerController* Controller = Scene.World->SpawnActor<APlayerController>();
		Controller->SetAsLocalPlayerController();
		Controller->Possess(Character);
		AChopItQuotaMachine* Machine = Scene.World->SpawnActorDeferred<AChopItQuotaMachine>(
			AChopItQuotaMachine::StaticClass(), FTransform::Identity);
		Machine->SetChainDefinition(Scene.Definition);
		Machine->FinishSpawning(FTransform::Identity);
		if (!Machine->HasActorBegunPlay()) Machine->DispatchBeginPlay();
		UChopItRopeComponent* Rope = Machine->FindComponentByClass<UChopItRopeComponent>();
		if (!TestTrue(*FString::Printf(TEXT("Scenario %d initializes its rope"), Scenario), Rope && Rope->IsInitialized())) continue;
		Character->GetCharacterMovement()->DisableMovement();
		const TArray<FVector> PositionsBeforeDeath = Rope->GetParticleLocations();
		const int32 ContactsBeforeDeath = Rope->GetContacts().Num();
		const float LengthBeforeDeath = Rope->GetRopeLength();
		Machine->BeginDeathSequenceForAutomation();
		TestTrue(*FString::Printf(TEXT("Scenario %c reuses its rope"), TCHAR('A' + Scenario)),
			Rope == Machine->FindComponentByClass<UChopItRopeComponent>() && Rope->IsDeathRetracting());
		TestTrue(*FString::Printf(TEXT("Scenario %c starts at current length"), TCHAR('A' + Scenario)),
			FMath::IsNearlyEqual(LengthBeforeDeath, Machine->GetDeathDesiredRopeLengthForAutomation()));
		TestEqual(*FString::Printf(TEXT("Scenario %c preserves contact count"), TCHAR('A' + Scenario)),
			Rope->GetContacts().Num(), ContactsBeforeDeath);
		bool bPositionsPreserved = Rope->GetParticleLocations().Num() == PositionsBeforeDeath.Num();
		if (bPositionsPreserved)
			for (int32 Index = 0; Index < PositionsBeforeDeath.Num(); ++Index)
				bPositionsPreserved &= Rope->GetParticleLocations()[Index].Equals(PositionsBeforeDeath[Index], 0.001f);
		TestTrue(*FString::Printf(TEXT("Scenario %c preserves every segment at transition"), TCHAR('A' + Scenario)),
			bPositionsPreserved);
		if (Scenario == 0)
		{
			Scene.Definition->DeathRetractionMinSpeed = 0.0f;
			Scene.Definition->DeathRetractionMaxSpeed = 0.0f;
			const float XBeforeIdle = Character->GetActorLocation().X;
			for (int32 IdleFrame = 0; IdleFrame < 10; ++IdleFrame)
			{
				++GFrameCounter;
				Scene.World->Tick(LEVELTICK_All, 1.0f / 60.0f);
			}
			TestTrue(TEXT("Death without chain retraction does not pull the player"),
				FMath::Abs(Character->GetActorLocation().X - XBeforeIdle) < 1.0f);
			Scene.Definition->DeathRetractionMinSpeed = 300.0f;
			Scene.Definition->DeathRetractionMaxSpeed = 1800.0f;
		}
		if (Scenario == 4)
		{
			// Build a sealed pocket after deployment to exercise the delayed failsafe.
			Scene.Box(FVector(465, 0, 110), FVector(20, 110, 110));
			Scene.Box(FVector(635, 0, 110), FVector(20, 110, 110));
			Scene.Box(FVector(550, 110, 110), FVector(85, 20, 110));
			Scene.Box(FVector(550, -110, 110), FVector(85, 20, 110));
		}
		float MaximumAttachmentError = 0.0f;
		float MaximumLateralTravel = 0.0f;
		float ClearPull = 0.0f, ClearReel = 0.0f;
		float PreviousDistance = FVector::Dist2D(Character->GetActorLocation(), Machine->GetActorLocation());
		float PreviousLength = Rope->GetRopeLength();
		bool bConsumed = false;
		bool bStalled = false;
		TArray<float> DeathUpdateTimes;
		TArray<float> RopeUpdateTimes;
		int64 TotalDeathQueries = 0;
		int32 PeakDeathQueries = 0;
		int64 TotalRopeSweeps = 0;
		int64 PeakRopeSweeps = 0;
		int32 PeakRopeIterations = 0;
		float FirstRetractionSpeed = -1.0f;
		float SpeedAfterThirtyFrames = -1.0f;
		bool bLengthFollowsSpeed = true;
		const float Dt = Scenario == 5 ? 0.15f : 1.0f / 60.0f;
		for (int32 Frame = 0; Frame < 720; ++Frame)
		{
			const float PreviousTarget = Machine->GetDeathDesiredRopeLengthForAutomation();
			++GFrameCounter;
			Scene.World->Tick(LEVELTICK_All, Dt);
			const float CurrentSpeed = Machine->GetDeathRetractionSpeedForAutomation();
			if (Frame == 0) FirstRetractionSpeed = CurrentSpeed;
			if (Frame == 29) SpeedAfterThirtyFrames = CurrentSpeed;
			bLengthFollowsSpeed &= FMath::IsNearlyEqual(Machine->GetDeathDesiredRopeLengthForAutomation(),
				FMath::Max(Scene.Definition->MinimumDeployedLength, PreviousTarget - CurrentSpeed * Dt), 0.05f);
			DeathUpdateTimes.Add(Machine->GetDeathUpdateMillisecondsForAutomation());
			RopeUpdateTimes.Add(Rope->GetLastSimulationMilliseconds());
			TotalDeathQueries += Machine->GetDeathCollisionQueriesForAutomation();
			PeakDeathQueries = FMath::Max(PeakDeathQueries, Machine->GetDeathCollisionQueriesForAutomation());
			TotalRopeSweeps += Rope->GetSweepQueriesThisFrame();
			PeakRopeSweeps = FMath::Max(PeakRopeSweeps, Rope->GetSweepQueriesThisFrame());
			PeakRopeIterations = FMath::Max(PeakRopeIterations, Rope->GetIterationsThisFrame());
			if (Machine->IsDeathPresentationReady() && Rope->IsInitialized()) { bStalled = true; break; }
			if (!Rope->IsInitialized()) { bConsumed = !Character->GetActorEnableCollision(); break; }
			const float Distance = FVector::Dist2D(Character->GetActorLocation(), Machine->GetActorLocation());
			MaximumLateralTravel = FMath::Max(MaximumLateralTravel,
				FMath::Abs(static_cast<float>(Character->GetActorLocation().Y)));
			const float Length = Rope->GetRopeLength();
			const float ClearSpanBoundary = Scenario == 5 ? 200.0f : 350.0f;
			if ((Scenario == 0 || Scenario == 5) && PreviousDistance > ClearSpanBoundary && Distance > ClearSpanBoundary)
			{
				ClearPull += FMath::Max(0.0f, PreviousDistance - Distance);
				ClearReel += FMath::Max(0.0f, PreviousLength - Length);
			}
			PreviousDistance = Distance;
			PreviousLength = Length;
			const FVector Anchor = Character->GetActorTransform().TransformPosition(Scene.Definition->PlayerChainAnchor);
			MaximumAttachmentError = FMath::Max(MaximumAttachmentError,
				static_cast<float>(FVector::Distance(Anchor, Rope->GetAcceptedEndpoint())));
		}
		DeathUpdateTimes.Sort();
		RopeUpdateTimes.Sort();
		const int32 SampleCount = DeathUpdateTimes.Num();
		const int32 P95Index = FMath::Clamp(FMath::FloorToInt(SampleCount * 0.95f), 0, SampleCount - 1);
		AddInfo(FString::Printf(TEXT("Death scenario %c: consumed %d stalled %d, endpoint error %.2f cm, lateral %.1f cm, clear approach %.1f cm, reel %.1f cm"),
			TCHAR('A' + Scenario), bConsumed, bStalled, MaximumAttachmentError, MaximumLateralTravel, ClearPull, ClearReel));
		AddInfo(FString::Printf(TEXT("Death profile %c: frames %d, death P95 %.3f ms, rope P95 %.3f ms, death queries avg %.1f peak %d, rope sweeps avg %.1f peak %lld, rope iterations peak %d"),
			TCHAR('A' + Scenario), SampleCount, DeathUpdateTimes[P95Index], RopeUpdateTimes[P95Index],
			SampleCount ? static_cast<double>(TotalDeathQueries) / SampleCount : 0.0, PeakDeathQueries,
			SampleCount ? static_cast<double>(TotalRopeSweeps) / SampleCount : 0.0, PeakRopeSweeps, PeakRopeIterations));
		TestTrue(*FString::Printf(TEXT("Scenario %c completes or reports a blocked capsule"), TCHAR('A' + Scenario)),
			bConsumed || bStalled);
		TestTrue(*FString::Printf(TEXT("Scenario %c integrates its current speed"), TCHAR('A' + Scenario)), bLengthFollowsSpeed);
		if (Scenario == 6)
			TestTrue(TEXT("Curve increases the death reel speed over time"),
				FirstRetractionSpeed >= Scene.Definition->DeathRetractionMinSpeed
				&& SpeedAfterThirtyFrames > FirstRetractionSpeed + 100.0f);
		TestTrue(*FString::Printf(TEXT("Scenario %c keeps chain attached"), TCHAR('A' + Scenario)), MaximumAttachmentError < 5.0f);
		if (Scenario == 0 || Scenario == 5)
			TestTrue(*FString::Printf(TEXT("Scenario %c reel keeps up with approach"), TCHAR('A' + Scenario)),
				ClearPull > 50.0f && ClearReel >= ClearPull * 0.95f);
		if (Scenario == 0 || Scenario == 5 || Scenario == 6)
			TestTrue(*FString::Printf(TEXT("Clear scenario %c reaches the furnace"), TCHAR('A' + Scenario)), bConsumed);
		if (Scenario == 1)
			TestTrue(*FString::Printf(TEXT("Scenario %c slides around collision"), TCHAR('A' + Scenario)),
				MaximumLateralTravel > 5.0f || bStalled);
		if (Scenario == 4)
			TestTrue(TEXT("Trapped capsule ends in place without a recovery teleport"),
				bStalled && Machine->GetDeathStallCountForAutomation() > 0);
	}
	return true;
}
