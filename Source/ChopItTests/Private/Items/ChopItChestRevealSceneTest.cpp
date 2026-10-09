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
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "MaterialShared.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "UI/ChopItHUD.h"
#include "Items/ChopItItemDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "UnrealClient.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "ShaderCompiler.h"

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
	if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
	TestNotNull(TEXT("Dedicated render target"),Scene->GetTexture());
	TestNotNull(TEXT("UI material restores foreground opacity from capture alpha"),Scene->GetCaptureCompositeMaterial());
	const FMaterialResource* MaterialResource = Scene->GetCaptureCompositeMaterial()->GetMaterial()->GetMaterialResource(GMaxRHIShaderPlatform);
	if (TestNotNull(TEXT("Composite shader resource"), MaterialResource))
		for (const FString& Error : MaterialResource->GetCompileErrors()) AddError(Error);
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
			TestTrue(TEXT("HDR capture preserves both clear background and inverted foreground alpha for compositing"),
				Pixels.ContainsByPredicate([](const FColor& P){ return P.A < 16; })
				&& Pixels.ContainsByPredicate([](const FColor& P){ return P.A > 240; }));
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(1024,1024,Pixels,PNG);
			FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectSavedDir()+FString::Printf(TEXT("ChestRevealTier%d.png"),Tier)));
		}
		else AddError(TEXT("Could not read the chest render target. Run this test with a rendering RHI."));

		// Exercise the actual HUD Canvas material path, not only the source capture.
		UTextureRenderTarget2D* Composed = NewObject<UTextureRenderTarget2D>(Scene);
		Composed->InitCustomFormat(1024,1024,PF_B8G8R8A8,false);
		Composed->UpdateResourceImmediate(true);
		FCanvas Canvas(Composed->GameThread_GetRenderTargetResource(),nullptr,World,World->GetFeatureLevel());
		Canvas.Clear(FLinearColor(.08f,.16f,.3f,1));
		Canvas.Flush_GameThread(true);
		FlushRenderingCommands();
		TArray<FColor> Before;
		Composed->GameThread_GetRenderTargetResource()->ReadPixels(Before);
		FCanvasTileItem Portrait(FVector2D::ZeroVector,Scene->GetCaptureCompositeMaterial()->GetRenderProxy(),FVector2D(1024));
		Portrait.SetColor(FLinearColor::White);
		Portrait.BlendMode=SE_BLEND_AlphaComposite;
		Canvas.DrawItem(Portrait);
		Canvas.Flush_GameThread(true);
		FlushRenderingCommands();
		TArray<FColor> After;
		if (Composed->GameThread_GetRenderTargetResource()->ReadPixels(After) && After.Num()==Before.Num() && After.Num()==1024*1024)
		{
			TestEqual(TEXT("Composite preserves background corner"),After[0],Before[0]);
			int32 Changed=0;
			for(int32 I=0;I<After.Num();++I) if(After[I]!=Before[I]) ++Changed;
			TestTrue(TEXT("Canvas renders visible chest without an opaque rectangle"),Changed>10000 && Changed<800000);
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(1024,1024,After,PNG);
			FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectSavedDir()+FString::Printf(TEXT("ChestCompositeTier%d.png"),Tier)));
		}
		else AddError(TEXT("Cannot read composed HUD material"));
	}
	Scene->Destroy();
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItChestRevealPSXTest, "ChopIt.Items.ChestRevealPSX",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)
bool FChopItChestRevealPSXTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/ChopIt/World/Maps/L_PSX_test")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		UWorld* World=GEditor->PlayWorld;
		if (!TestNotNull(TEXT("PSX PIE world"),World)) return true;
		APlayerController* PC=World->GetFirstPlayerController();
		AChopItHUD* HUD=PC ? Cast<AChopItHUD>(PC->GetHUD()) : nullptr;
		if (!TestNotNull(TEXT("PSX actual HUD"),HUD)) return true;
		UChopItItemDataAsset* Item=NewObject<UChopItItemDataAsset>(HUD);
		Item->ItemId=TEXT("MandrakeStick");
		Item->Rarity=static_cast<EChopItItemRarity>(3);
		Item->DisplayName=FText::FromString(TEXT("Chest render regression"));
		HUD->StartItemReveal(Item);
		FAssetCompilingManager::Get().FinishAllCompilation();
		TestTrue(TEXT("Chest pauses PSX gameplay"),UGameplayStatics::IsGamePaused(World));
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
	{
		FViewport* Viewport=GEditor->PlayWorld->GetGameViewport()->Viewport;
		TArray<FColor> Screen;
		if (TestTrue(TEXT("Read actual PSX HUD pixels"),Viewport->ReadPixels(Screen)))
		{
			const FIntPoint Size=Viewport->GetSizeXY();
			int32 ChestPixels=0;
			for(int32 Y=Size.Y*3/10;Y<Size.Y*9/10;++Y)
				for(int32 X=Size.X*3/10;X<Size.X*7/10;++X)
				{
					const FColor P=Screen[Y*Size.X+X];
					if(P.R>90 && P.R>P.G*1.1f && P.R>P.B*1.2f) ++ChestPixels;
				}
			TestTrue(TEXT("Gold/wood chest is visible in final PSX HUD, not only its capture"),ChestPixels>500);
		}
		for (TActorIterator<AChopItChestRevealScene> It(GEditor->PlayWorld);It;++It)
		{
			FlushRenderingCommands();
			TArray<FColor> Pixels;
			It->GetTexture()->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
			TestTrue(TEXT("Actual paused PSX capture contains foreground"),Pixels.ContainsByPredicate([](const FColor& P){return P.A<128;}));
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(1024,1024,Pixels,PNG);
			FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectSavedDir()/TEXT("ChestRevealPSX_Raw.png")));
			AddInfo(FString::Printf(TEXT("PSX chest location %s; capture %s"),*It->GetActorLocation().ToString(),*It->FindComponentByClass<USceneCaptureComponent2D>()->GetComponentLocation().ToString()));
		}
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ChestRevealPSX.png"),false,false);
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
