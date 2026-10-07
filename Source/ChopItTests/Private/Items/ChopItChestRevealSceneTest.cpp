#include "Rewards/ChopItChestRevealScene.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "AssetCompilingManager.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItChestRevealSceneTest, "ChopIt.Items.ChestRevealScene",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)
bool FChopItChestRevealSceneTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
	AChopItChestRevealScene* Scene = World->SpawnActor<AChopItChestRevealScene>();
	Scene->InitializeScene();
	FAssetCompilingManager::Get().FinishAllCompilation();
	TestNotNull(TEXT("Dedicated render target"),Scene->GetTexture());
	for (int32 Tier=0; Tier<4; ++Tier)
	{
		const float Duration=.45f+.82f*(Tier+1)+.55f;
		Scene->UpdatePresentation(Duration-.25f,Duration,Tier);
		World->SendAllEndOfFrameUpdates();
		Scene->FindComponentByClass<USceneCaptureComponent2D>()->CaptureScene();
		FlushRenderingCommands();
		TArray<FColor> Pixels;
		if (Scene->GetTexture()->GameThread_GetRenderTargetResource()->ReadPixels(Pixels) && Pixels.Num()==1024*1024)
		{
			TestTrue(TEXT("Capture contains the rendered chest"), Pixels.ContainsByPredicate([](const FColor& P){ return P.R>80 || P.G>80 || P.B>80; }));
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(1024,1024,Pixels,PNG);
			FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectSavedDir()+FString::Printf(TEXT("ChestRevealTier%d.png"),Tier)));
		}
		else AddError(TEXT("Could not read the chest render target. Run this test with a rendering RHI."));
	}
	Scene->Destroy();
	World->DestroyWorld(false);
	return true;
}
#endif
