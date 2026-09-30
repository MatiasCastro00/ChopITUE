#if WITH_EDITOR
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PrimitiveComponent.h"
#include "ChopItCollision.h"
#include "Containers/Ticker.h"
#include "Economy/ChopItQuotaMachine.h"
#include "Economy/ChopItRopeComponent.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"

// Opt-in review capture, excluded from the game build. A separate scene capture
// avoids the introduction's paused dialogue camera overriding the review view.
static FAutoConsoleCommandWithWorldAndArgs CaptureChainV2(
	TEXT("ChopIt.Chain.Capture"), TEXT("Capture the physical chain after settling; optional 'exit' for unattended QA."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld()) return;
		const bool bExit = Args.Contains(TEXT("exit"));
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(World, [World, bExit](float)
		{
			APlayerController* PC = World->GetFirstPlayerController();
			const bool bPaused = World->IsPaused();
			if (PC) PC->SetPause(false);
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(World, [World, bExit, bPaused](float)
			{
				FBox Bounds(ForceInit);
				for (TActorIterator<AChopItQuotaMachine> It(World); It; ++It)
					if (UChopItRopeComponent* Rope = It->FindComponentByClass<UChopItRopeComponent>())
					{
						UE_LOG(LogTemp, Display, TEXT("Chain capture %s: %s"), *It->GetName(), *Rope->DescribeState());
						for (const FVector& Point : Rope->GetParticleLocations()) Bounds += Point;
					}
				if (Bounds.IsValid)
				{
					const FVector Centre = Bounds.GetCenter();
					const FVector Location = Centre + FVector(0.65, -0.65, 1) * FMath::Max(650.0, Bounds.GetExtent().Size() * 1.6);
					ASceneCapture2D* Camera = World->SpawnActor<ASceneCapture2D>(Location, (Centre - Location).Rotation());
					USceneCaptureComponent2D* Capture = Camera->GetCaptureComponent2D();
					UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(Camera);
					Target->InitCustomFormat(1280, 720, PF_B8G8R8A8, false);
					Capture->TextureTarget = Target;
					Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
					Capture->bCaptureEveryFrame = false;
					Capture->FOVAngle = 75;
					Capture->CaptureScene();
					const FString Folder = FPaths::ProjectSavedDir() / TEXT("ChainV2Visuals");
					IFileManager::Get().MakeDirectory(*Folder, true);
					TUniquePtr<FArchive> File(IFileManager::Get().CreateFileWriter(*(Folder / (World->GetMapName() + TEXT(".png")))));
					if (File) FImageUtils::ExportRenderTarget2DAsPNG(Target, *File);
					Camera->Destroy();
				}
				if (APlayerController* Controller = World->GetFirstPlayerController()) Controller->SetPause(bPaused);
				if (bExit) FPlatformMisc::RequestExit(false);
				return false;
			}), 3.0f);
			return false;
		}), 5.0f);
	}));

// Opt-in replay in the saved map with its actual pawn, machine Blueprint and
// world collision. Does not save or alter map assets. Run in a separate game.
static FAutoConsoleCommandWithWorldAndArgs ReplayChainReturn(
	TEXT("ChopIt.Chain.ReplayReturn"), TEXT("Replay a return walk; optional 'y', 'post' (ChainLab two laps), and 'exit'."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld()) return;
		const bool bExit = Args.Contains(TEXT("exit"));
		const FVector OutDirection = Args.Contains(TEXT("y")) ? FVector::RightVector : -FVector::ForwardVector;
		const bool bPost = Args.Contains(TEXT("post")) && World->GetMapName().Contains(TEXT("ChainLab"));
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(World, [World, bExit, OutDirection, bPost](float)
		{
			APlayerController* PC = World->GetFirstPlayerController();
			ACharacter* Player = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
			UChopItRopeComponent* Rope = nullptr;
			for (TActorIterator<AChopItQuotaMachine> It(World); It; ++It)
				if (auto* Candidate = It->FindComponentByClass<UChopItRopeComponent>(); Candidate && Candidate->IsInitialized()) { Rope = Candidate; break; }
			if (!Player || !Rope) { UE_LOG(LogTemp, Error, TEXT("ChainReplay: missing pawn/chain")); if (bExit) FPlatformMisc::RequestExit(false); return false; }
			const FVector Origin = Player->GetActorLocation();
			UE_LOG(LogTemp, Display, TEXT("ChainReplay start map=%s pawn=%s origin=%s speed=%.1f"), *World->GetMapName(), *Player->GetClass()->GetName(), *Origin.ToString(), Player->GetCharacterMovement()->MaxWalkSpeed);
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(World,
				[World, bExit, OutDirection, bPost, Player = TWeakObjectPtr<ACharacter>(Player), Rope = TWeakObjectPtr<UChopItRopeComponent>(Rope), Origin, Time = 0.0f, NextLog = 0.0f, Slow = 0, Samples = 0, Waypoint = 0, FramesSinceTurn = 0, Times = TArray<float>(), bClear = true](float) mutable
			{
				if (!Player.IsValid() || !Rope.IsValid()) return false;
				if (auto* Controller = World->GetFirstPlayerController()) Controller->SetPause(false);
				const float Dt = World->GetDeltaSeconds();
				Time += Dt;
				if (bPost)
				{
					const FVector Route[] = {FVector(-540,-1030,0), FVector(-140,-1030,0), FVector(-140,-630,0), FVector(-540,-630,0)};
					FVector Offset = Route[Waypoint % 4] - Player->GetActorLocation(); Offset.Z = 0;
					if (Offset.Size() < 35) { ++Waypoint; FramesSinceTurn = 0; Offset = Route[Waypoint % 4] - Player->GetActorLocation(); Offset.Z = 0; }
					++FramesSinceTurn;
					Times.Add(Rope->GetLastSimulationMilliseconds());
					if (FramesSinceTurn * Dt > .35f && Offset.Size() > 100) {
						++Samples; if (Player->GetCharacterMovement()->Velocity.Size2D() < 500) ++Slow;
					}
					bClear &= Rope->IsCollisionFree(0.5f);
					if (Time >= NextLog)
					{
						UE_LOG(LogTemp, Display, TEXT("ChainPost t=%.2f waypoint=%d/9 speed=%.1f %s"), Time, Waypoint, Player->GetCharacterMovement()->Velocity.Size2D(), *Rope->DescribeState());
						NextLog += .5f;
					}
					if (Waypoint < 9 && Time < 20) { Player->AddMovementInput(Offset.GetSafeNormal(), 1, true); return true; }
					float Winding = 0;
					const auto& Points = Rope->GetParticleLocations();
					for (int32 I = 1; I < Points.Num(); ++I) {
						const FVector2D A(Points[I-1].X + 340, Points[I-1].Y + 830), B(Points[I].X + 340, Points[I].Y + 830);
						Winding += FMath::Atan2(A.X * B.Y - A.Y * B.X, FVector2D::DotProduct(A, B));
					}
					Times.Sort();
					const float P95 = Times[FMath::Min(Times.Num()-1, FMath::FloorToInt(Times.Num() * .95f))];
					UE_LOG(LogTemp, Display, TEXT("ChainPost FINISH waypoints=%d/9 clear=%d winding=%.3f stock=%.1f slow=%d/%d P95=%.2fms"), Waypoint, bClear, Winding, Rope->GetStoredLength(), Slow, Samples, P95);
					if (bExit) FPlatformMisc::RequestExit(false);
					return false;
				}
				const float Speed = FVector::DotProduct(Player->GetCharacterMovement()->Velocity, -OutDirection);
				if (Time > 3.0f && Time < 4.8f) { ++Samples; if (Speed < 500) ++Slow; }
				const bool bCurrentlyClear = Rope->IsCollisionFree(0.5f);
				if (bClear && !bCurrentlyClear)
				{
					FCollisionQueryParams Query(SCENE_QUERY_STAT(ChainReplayContact), false, Rope->GetOwner());
					Query.AddIgnoredActor(Player.Get());
					FCollisionResponseParams Response = FCollisionResponseParams::DefaultResponseParam;
					for (ECollisionChannel Channel : {ECC_Pawn, ChopItCollisionChannels::Enemy, ChopItCollisionChannels::Projectile,
						ChopItCollisionChannels::Pickup, ChopItCollisionChannels::DeliveryZone, ChopItCollisionChannels::Chain})
						Response.CollisionResponse.SetResponse(Channel, ECR_Ignore);
					const auto& Points = Rope->GetParticleLocations();
					for (int32 I = 1; I < Points.Num(); ++I) {
						FHitResult Hit;
						if (World->SweepSingleByChannel(Hit, Points[I-1], Points[I], FQuat::Identity, ChopItCollisionChannels::Chain,
							FCollisionShape::MakeSphere(Rope->GetCollisionRadius() - .5f), Query, Response)) {
							UE_LOG(LogTemp, Warning, TEXT("ChainReplay BLOCKED t=%.3f actor=%s class=%s component=%s segment=%d A=%s B=%s depth=%.3f normal=%s"),
								Time, *GetNameSafe(Hit.GetActor()), Hit.GetActor() ? *Hit.GetActor()->GetClass()->GetName() : TEXT("none"),
								*GetNameSafe(Hit.GetComponent()), I-1, *Points[I-1].ToString(), *Points[I].ToString(), Hit.PenetrationDepth, *Hit.Normal.ToString());
							break;
						}
					}
				}
				bClear &= bCurrentlyClear;
				if (Time >= NextLog) {
					UE_LOG(LogTemp, Display, TEXT("ChainReplay t=%.2f distance=%.2f speed=%.2f %s"), Time, FVector::DotProduct(Player->GetActorLocation() - Origin, -OutDirection), Speed, *Rope->DescribeState());
					NextLog += .5f;
				}
				if (Time < 5.0f) Player->AddMovementInput(Time < 2.5f ? OutDirection : -OutDirection, 1, true);
				if (Time < 7.0f) return true;
				UE_LOG(LogTemp, Display, TEXT("ChainReplay FINISH returnDelta=%.2f slow=%d/%d clear=%d"), FVector::DotProduct(Player->GetActorLocation() - Origin, -OutDirection), Slow, Samples, bClear);
				if (bExit) FPlatformMisc::RequestExit(false);
				return false;
			}));
			return false;
		}), 5.0f);
	}));

// Opt-in real-map profiler for the complete death controller. It exercises the
// authored pawn, machine, collision and renderer, then exits without saving.
static FAutoConsoleCommandWithWorldAndArgs ProfileChainDeath(
	TEXT("ChopIt.Chain.ProfileDeath"), TEXT("Profile the Furnace death pull in the running map; optional 'exit'."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld()) return;
		const bool bExit = Args.Contains(TEXT("exit"));
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(World, [World, bExit](float)
		{
			if (APlayerController* Controller = World->GetFirstPlayerController()) Controller->SetPause(false);
			ACharacter* Player = World->GetFirstPlayerController()
				? Cast<ACharacter>(World->GetFirstPlayerController()->GetPawn()) : nullptr;
			AChopItQuotaMachine* Machine = nullptr;
			for (TActorIterator<AChopItQuotaMachine> It(World); It; ++It)
				if (UChopItRopeComponent* Rope = It->FindComponentByClass<UChopItRopeComponent>();
					Rope && Rope->IsInitialized()) { Machine = *It; break; }
			if (!Player || !Machine)
			{
				UE_LOG(LogTemp, Error, TEXT("DeathProfile: missing authored pawn or initialized Furnace chain"));
				if (bExit) FPlatformMisc::RequestExit(false);
				return false;
			}
			Player->GetCharacterMovement()->StopMovementImmediately();
			Player->GetCharacterMovement()->DisableMovement();
			UE_LOG(LogTemp, Display, TEXT("DeathProfile START player=%s machine=%s distance=%.1f"),
				*Player->GetActorLocation().ToString(), *Machine->GetActorLocation().ToString(),
				FVector::Dist(Player->GetActorLocation(), Machine->GetActorLocation()));
			Machine->BeginDeathSequenceForAutomation();
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(World,
				[World, bExit, Player = TWeakObjectPtr<ACharacter>(Player), Machine = TWeakObjectPtr<AChopItQuotaMachine>(Machine),
				Time = 0.0f, FrameTimes = TArray<float>(), DeathTimes = TArray<float>(), PeakQueries = 0](float DeltaTime) mutable
			{
				if (!Player.IsValid() || !Machine.IsValid()) return false;
				if (APlayerController* Controller = World->GetFirstPlayerController()) Controller->SetPause(false);
				Time += DeltaTime;
				FrameTimes.Add(DeltaTime * 1000.0f);
				DeathTimes.Add(Machine->GetDeathUpdateMillisecondsForAutomation());
				PeakQueries = FMath::Max(PeakQueries, Machine->GetDeathCollisionQueriesForAutomation());
				if (Player->GetActorEnableCollision() && Time < 15.0f) return true;
				FrameTimes.Sort(); DeathTimes.Sort();
				const int32 P95 = FMath::Clamp(FMath::FloorToInt(FrameTimes.Num() * 0.95f), 0, FrameTimes.Num() - 1);
				const float P95FrameMs = FrameTimes[P95];
				UE_LOG(LogTemp, Display, TEXT("DeathProfile FINISH consumed=%d frames=%d P95Frame=%.3fms P95FPS=%.1f P95Death=%.3fms peakQueries=%d"),
					!Player->GetActorEnableCollision(), FrameTimes.Num(), P95FrameMs,
					1000.0f / FMath::Max(0.001f, P95FrameMs), DeathTimes[P95], PeakQueries);
				if (bExit) FPlatformMisc::RequestExit(false);
				return false;
			}));
			return false;
		}), 5.0f);
	}));
#endif
