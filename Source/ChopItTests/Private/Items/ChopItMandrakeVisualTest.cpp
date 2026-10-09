#include "Items/ChopItMandrakeScream.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "ContentStreaming.h"
#include "StaticMeshResources.h"
#include "NiagaraSystemInstanceController.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraEmitterInstance.h"
#include "Materials/Material.h"
#include "MaterialShared.h"
#include "Engine/Engine.h"
#include "Materials/MaterialRenderProxy.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItMandrakeVisualTest,"ChopIt.Items.MandrakeVisual",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)
bool FChopItMandrakeVisualTest::RunTest(const FString& Parameters)
{
 const auto Values=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 auto* Actor=World->SpawnActor<AChopItMandrakeScream>();
 Actor->Initialize(Actor,350,8,6);
 Actor->Tick(.3f);
 auto* Confusion=NewObject<UNiagaraComponent>(Actor);
 Actor->AddInstanceComponent(Confusion);Confusion->SetupAttachment(Actor->GetRootComponent());
 Confusion->SetRelativeLocation(FVector(0,0,125));
 Confusion->SetAsset(LoadObject<UNiagaraSystem>(nullptr,TEXT("/Game/ChopIt/Items/Mandrake/NS_Mandrake_Confusion.NS_Mandrake_Confusion")));
 Confusion->RegisterComponent();
 auto* Body=Actor->FindComponentByClass<UStaticMeshComponent>();
 for(const auto& Section:Body->GetStaticMesh()->GetRenderData()->LODResources[0].Sections)
  AddInfo(FString::Printf(TEXT("Mesh section material %d: %s"),Section.MaterialIndex,*GetNameSafe(Body->GetMaterial(Section.MaterialIndex))));
 auto* Floor=World->SpawnActor<AStaticMeshActor>();
 Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 Floor->SetActorLocation(FVector(0,0,-6));Floor->SetActorScale3D(FVector(9,9,.1));
 auto* Light=NewObject<UPointLightComponent>(Floor);Light->RegisterComponent();
 Light->SetWorldLocation(FVector(100,-100,220));Light->SetIntensity(10000);Light->SetAttenuationRadius(1200);
 auto* Capture=NewObject<USceneCaptureComponent2D>(Actor);Capture->RegisterComponent();
 auto* RT=NewObject<UTextureRenderTarget2D>();RT->InitAutoFormat(1024,1024);Capture->TextureTarget=RT;
 Capture->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR;
 Capture->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
 Capture->ShowOnlyActors.Add(Actor);Capture->ShowOnlyActors.Add(Floor);
 Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;
 Capture->bAlwaysPersistRenderingState=true;
 Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
 Capture->PostProcessSettings.bOverride_AutoExposureMethod=true;Capture->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
 Capture->PostProcessSettings.bOverride_AutoExposureBias=true;Capture->PostProcessSettings.AutoExposureBias=0;
 Capture->SetWorldLocation(FVector(210,-155,160));Capture->SetWorldRotation((FVector(0,0,45)-Capture->GetComponentLocation()).Rotation());
 Capture->FOVAngle=45;
 FAssetCompilingManager::Get().FinishAllCompilation();
 for(const TCHAR* Name:{TEXT("M_Mandrake_Body"),TEXT("M_Mandrake_TearsFlow"),TEXT("M_Mandrake_SonicRefraction"),TEXT("M_Mandrake_ConfusionStar"),TEXT("M_Mandrake_Droplet")})
 if(auto* Mat=LoadObject<UMaterial>(nullptr,*(FString(TEXT("/Game/ChopIt/Items/Mandrake/"))+Name)))Mat->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Synchronous);
 if(GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
 for(int32 Slot=0;Slot<2;++Slot){auto* Mat=Body->GetMaterial(Slot)->GetMaterial();auto* Resource=Mat->GetMaterialResource(GMaxRHIShaderPlatform);AddInfo(FString::Printf(TEXT("Material %s shader=%d domain=%d materials=%d feature=%d"),*Mat->GetName(),Resource&&Resource->GetGameThreadShaderMap()!=nullptr,int(Mat->MaterialDomain),Capture->ShowFlags.Materials,int(World->GetFeatureLevel())));}
 IStreamingManager::Get().StreamAllResources(30.f);
 TArray<UNiagaraComponent*> Effects;Actor->GetComponents(Effects);
 for(auto* FX:Effects){TestNotNull(TEXT("Niagara asset assigned"),FX->GetAsset());if(FX->GetAsset()){FX->GetAsset()->WaitForCompilationComplete();TestTrue(TEXT("Niagara compiled"),FX->GetAsset()->IsValid());} FX->SetForceSolo(true);FX->ReinitializeSystem();FX->Activate(true);FX->AdvanceSimulation(42,1.f/60.f);
 auto Controller=FX->GetSystemInstanceController();int32 Count=0;
 if(Controller.IsValid())for(const auto& Emitter:Controller->GetSystemInstance_Unsafe()->GetEmitters())Count+=Emitter->GetNumParticles();
 AddInfo(FString::Printf(TEXT("%s particles=%d"),*FX->GetName(),Count));TestTrue(TEXT("Niagara spawns particles"),Count>0);
 }
 Body->MarkRenderStateDirty();
 FString RenderMaterial;
 auto* Proxy=Body->GetMaterial(0)->GetRenderProxy();
 ENQUEUE_RENDER_COMMAND(MandrakeMaterialCheck)([Proxy,&RenderMaterial](FRHICommandListImmediate&){const FMaterialRenderProxy* Fallback=nullptr;const FMaterial& Used=Proxy->GetMaterialWithFallback(GMaxRHIFeatureLevel,Fallback);RenderMaterial=FString::Printf(TEXT("Actual render material %s default=%d"),*Used.GetFriendlyName(),Used.IsDefaultMaterial());});
 FlushRenderingCommands();AddInfo(RenderMaterial);TestFalse(TEXT("Body uses authored shader"),RenderMaterial.Contains(TEXT("default=1")));
 for(int32 Frame=0;Frame<8;++Frame){++GFrameCounter;
 for(auto* FX:Effects){FX->TickComponent(1.f/60.f,LEVELTICK_All,nullptr);FX->MarkRenderDynamicDataDirty();}
 World->SendAllEndOfFrameUpdates();FlushRenderingCommands();Capture->CaptureScene();FlushRenderingCommands();
 if(GShaderCompilingManager){GShaderCompilingManager->ProcessAsyncResults(false,true);GShaderCompilingManager->FinishAllCompilation();}
 }
 TArray<FColor> Pixels;TestTrue(TEXT("Mandrake capture readable"),RT->GameThread_GetRenderTargetResource()->ReadPixels(Pixels));
 if(Pixels.Num()==1024*1024){TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(1024,1024,Pixels,PNG);FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectSavedDir()+TEXT("MandrakePreview.png")));}
 World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return true;
}
#endif

