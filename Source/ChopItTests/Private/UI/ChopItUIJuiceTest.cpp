#include "Misc/AutomationTest.h"
#include "UI/ChopItJuiceWidget.h"
#include "UI/ChopItUIJuiceProfile.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Combat/ChopItHealthComponent.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "Economy/ChopItQuotaComponent.h"
#include "Economy/ChopItEconomyComponent.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Progression/ChopItExperienceComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Economy/ChopItQuotaMachine.h"
#include "Economy/ChopItDeliveryZone.h"
#include "Components/SphereComponent.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItUIJuiceTest, "ChopIt.UI.Juice", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItUIJuiceTest::RunTest(const FString& Parameters)
{
 auto* UI=NewObject<UChopItJuiceWidget>();
 UI->JuiceProfile=NewObject<UChopItUIJuiceProfile>(); // No audio or world side effects.
 UI->WidgetTree=NewObject<UWidgetTree>(UI);
 auto* Root=UI->WidgetTree->ConstructWidget<UCanvasPanel>(); UI->WidgetTree->RootWidget=Root;
 for(FName Name:{FName("HealthFill"),FName("QuotaFill"),FName("XPFill")})
 {
  auto* Image=UI->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),Name);
  auto* Slot=Root->AddChildToCanvas(Image); Slot->SetSize(FVector2D(156,25));
  FSlateBrush Brush; Brush.SetUVRegion(FBox2d(FVector2d(.2,.3),FVector2d(.8,.4))); Image->SetBrush(Brush);
  UChopItJuiceWidget::FVisual Original; Original.Size=Slot->GetSize(); Original.Brush=Brush; UI->Originals.Add(Name,Original);
  for(float Percent:{0.f,.25f,.5f,1.f})
  {
   UI->SetFill(Name,Percent);
   TestEqual(TEXT("Clipping keeps original height"),Slot->GetSize().Y,25.);
   TestTrue(TEXT("Visible width follows percentage"),FMath::IsNearlyEqual(Slot->GetSize().X,156.*Percent));
   const FBox2f UV=Image->GetBrush().GetUVRegion();
   TestTrue(TEXT("Texture UV is cropped, not stretched"),FMath::IsNearlyEqual(UV.Max.X,float(.2+.6*Percent),.0001f));
  }
  TestEqual(TEXT("Original size is immutable"),UI->Originals[Name].Size,FVector2D(156,25));
 }
 UI->HealthChanged(100,100,nullptr); UI->CargoChanged(0,24); UI->QuotaChanged(0,200,false); UI->BalanceChanged(125,0);
 TestEqual(TEXT("Initialization emits no sounds"),UI->LastSound.Num(),0);
 TestEqual(TEXT("Initialization emits no punches"),UI->Punches.Num(),0);
 auto* Health=NewObject<UChopItHealthComponent>(); UI->Health=Health;
 Health->OnHealthChanged.AddUObject(UI,&UChopItJuiceWidget::HealthChanged);
 auto* Cargo=NewObject<UChopItWoodCargoComponent>(); UI->Cargo=Cargo;
 Cargo->OnCargoChanged.AddDynamic(UI,&UChopItJuiceWidget::CargoChanged);
 Cargo->OnPickupRejected.AddDynamic(UI,&UChopItJuiceWidget::Rejected);
 auto* Quota=NewObject<UChopItQuotaComponent>(); Quota->InitializeQuota(200); UI->Quota=Quota;
 Quota->OnQuotaChanged.AddDynamic(UI,&UChopItJuiceWidget::QuotaChanged);
 auto* Money=NewObject<UChopItEconomyComponent>(); UI->Economy=Money;
 Money->OnBalanceChanged.AddDynamic(UI,&UChopItJuiceWidget::BalanceChanged);
 UI->bReady=true;
 FChopItDamageSpec Damage; Damage.BaseDamage=20;
 Health->ApplyDamage(Damage,nullptr); Health->ApplyDamage(Damage,nullptr);
 TestEqual(TEXT("Consecutive real damage reaches UI"),UI->HealthValue,60.f);
 Health->ResetHealth(); TestEqual(TEXT("Real reset/healing reaches UI"),UI->HealthValue,100.f);
 Cargo->TryAddWood(1); Cargo->TryAddWood(1000); Cargo->TryAddWood(1);
 TestEqual(TEXT("Cargo gain follows actual capped cargo"),UI->Wood,Cargo->GetCurrentWood());
 TestTrue(TEXT("Rejected pickup has separate notification"),UI->LastSound.Contains("Reject"));
 Quota->TryContributeWood(FGuid::NewGuid(),50); TestEqual(TEXT("Partial quota event reaches UI"),UI->Progress,50);
 Quota->TryContributeWood(FGuid::NewGuid(),150); TestEqual(TEXT("Completion event reaches UI"),UI->CompleteAge,0.f);
 Quota->InitializeQuota(250); TestEqual(TEXT("New quota clears completion"),UI->CompleteAge,-1.f);
 Money->ApplyTransaction(FGuid::NewGuid(),TEXT("UIAutomation"),50);
 TestEqual(TEXT("Money event reaches UI"),UI->Balance,Money->GetBalance());
 UI->PhaseChanged(EChopItCyclePhase::Day,EChopItCyclePhase::Night,1);
 UI->ClockChanged(EChopItCyclePhase::Day,15); TestEqual(TEXT("25 percent amber"),UI->ClockStage,1);
 UI->ClockChanged(EChopItCyclePhase::Day,6); TestEqual(TEXT("10 percent red"),UI->ClockStage,2);
 UI->Time=10; UI->ClockChanged(EChopItCyclePhase::Day,0);
 TestTrue(TEXT("Deadline impact is emitted"),UI->LastSound.Contains("Expire"));
 UI->Time=11; UI->ClockChanged(EChopItCyclePhase::Day,0);
 TestEqual(TEXT("Zero is emitted only once per crossing"),UI->LastSound.FindRef("Expire"),10.f);
 UI->ClockChanged(EChopItCyclePhase::Night,1); TestEqual(TEXT("Night minimum has no urgency"),UI->ClockStage,0);
 auto* XP=NewObject<UChopItExperienceComponent>(); UI->Experience=XP;
 XP->OnExperienceChanged.AddDynamic(UI,&UChopItJuiceWidget::XPChanged);
 XP->AddExperience(1000);
 TestEqual(TEXT("Real multi-level event updates target"),UI->XPTargetLevel,XP->GetLevel());
 TestTrue(TEXT("Level change fills old bar first"),UI->XPTransition>=0 && UI->Fills["XPFill"].To==1.f);
 UI->Unbind(); Health->ApplyDamage(Damage,nullptr);
 TestEqual(TEXT("Unbound HUD no longer receives events"),UI->HealthValue,100.f);
 TestFalse(TEXT("Unbind resets ready state"),UI->bReady);
 auto* Icon=UI->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("SunIcon"));
 auto* IconSlot=Root->AddChildToCanvas(Icon); IconSlot->SetSize(FVector2D(58));
 UChopItJuiceWidget::FVisual IconOriginal; IconOriginal.Size=FVector2D(58); IconOriginal.Transform.Angle=7;
 IconOriginal.Brush.SetUVRegion(FBox2d(FVector2d(777./1774,691./887),FVector2d(955./1774,868./887)));
 UI->Originals.Add(TEXT("SunIcon"),IconOriginal);
 UI->UpdateClockIcon(EChopItCyclePhase::Day,true);
 TestEqual(TEXT("Day selects sun"),UI->ClockIconPhase,0);
 UI->UpdateClockIcon(EChopItCyclePhase::Dusk);
 UI->TickClockIcon(UI->JuiceProfile->ClockIconFadeDuration*.5f);
 TestEqual(TEXT("Icon changes at invisible fade midpoint"),Icon->GetRenderOpacity(),0.f);
 TestEqual(TEXT("Dusk selects sunset"),UI->ClockIconPhase,1);
 UI->TickClockIcon(UI->JuiceProfile->ClockIconFadeDuration*.5f);
 TestEqual(TEXT("Icon returns to original opacity"),Icon->GetRenderOpacity(),1.f);
 UI->UpdateClockIcon(EChopItCyclePhase::Night,true);
 TestEqual(TEXT("Night selects moon"),UI->ClockIconPhase,2);
 UI->Time=UI->JuiceProfile->ClockIconSwingPeriod*.25f; UI->ClockStage=0; UI->TickClockIcon(0);
 TestTrue(TEXT("Positive swing is relative +20 degrees"),FMath::IsNearlyEqual(Icon->GetRenderTransform().Angle,27.f));
 UI->Time=UI->JuiceProfile->ClockIconSwingPeriod*.75f; UI->TickClockIcon(0);
 TestTrue(TEXT("Negative swing is relative -20 degrees"),FMath::IsNearlyEqual(Icon->GetRenderTransform().Angle,-13.f));
 UI->JuiceProfile->bReducedMotion=true; UI->ClockStage=2; UI->TickClockIcon(0);
 TestTrue(TEXT("Reduced motion restores original icon transform"),Icon->GetRenderTransform()==IconOriginal.Transform);
 TestEqual(TEXT("Icon animation never resizes designer slot"),IconSlot->GetSize(),FVector2D(58));
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItUIJuicePIETest, "ChopIt.UI.JuicePIE", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItUIJuicePIETest::RunTest(const FString& Parameters)
{
 ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/ChopIt/World/Maps/L_PSX_test")));
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
 {
  UChopItJuiceWidget* UI=nullptr;
  for(TObjectIterator<UChopItJuiceWidget> It;It;++It) if(It->GetWorld()==GEditor->PlayWorld && It->IsInViewport()) { UI=*It; break; }
  if(!TestNotNull(TEXT("Actual L_PSX_test HUD uses presentation class"),UI)) return true;
  TestTrue(TEXT("Actual HUD binds to current pawn"),UI->bReady && UI->BoundPawn.IsValid());
  if(!TestTrue(TEXT("All real gameplay sources bound"),UI->Health.IsValid() && UI->Cargo.IsValid() && UI->Quota.IsValid() && UI->Economy.IsValid() && UI->Experience.IsValid() && UI->Cycle.IsValid())) return true;
  UI->Health->ResetHealth(); FChopItDamageSpec Damage; Damage.BaseDamage=20;
  UI->Health->ApplyDamage(Damage,nullptr); UI->Health->ApplyDamage(Damage,nullptr);
  TestEqual(TEXT("Real PIE damage updates HUD"),UI->HealthValue,60.f);
  for(int32 N=0;N<100;++N) UI->Cargo->TryAddWood(1);
  TestEqual(TEXT("Real PIE cargo capped"),UI->Wood,UI->Cargo->GetCurrentWood());
  UI->Economy->ApplyTransaction(FGuid::NewGuid(),TEXT("UIAutomation"),50);
  TestEqual(TEXT("Real PIE money updates HUD"),UI->Balance,UI->Economy->GetBalance());
  UI->Quota->InitializeQuota(200); UI->Quota->TryContributeWood(FGuid::NewGuid(),50);
  TestEqual(TEXT("Real PIE quota updates HUD"),UI->Progress,50);
  UI->RevealMission();
  TestTrue(TEXT("Deferred effects bounded under burst"),UI->Deferred.Num()<=16);
  TestTrue(TEXT("Particle and text hard caps"),UI->Particles.Num()<=56);
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.55f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([](){ FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("UIJuice/PIE_events.png"),true,false); return true; }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
 {
  for(TObjectIterator<UChopItJuiceWidget> It;It;++It) if(It->GetWorld()==GEditor->PlayWorld && It->IsInViewport())
  {
   for(const auto& Original:It->Originals) if(auto* W=It->Find(Original.Key); W && Original.Key!=TEXT("SunIcon"))
    TestTrue(*FString::Printf(TEXT("%s restores original transform"),*Original.Key.ToString()),W->GetRenderTransform()==Original.Value.Transform);
   TestTrue(TEXT("Burst effects retired"),It->Particles.IsEmpty());
   const float BeforePause=It->Time;
   UGameplayStatics::SetGamePaused(GEditor->PlayWorld,true); It->NativeTick(FGeometry(),.5f);
   TestEqual(TEXT("Paused world freezes presentation time"),It->Time,BeforePause);
   UGameplayStatics::SetGamePaused(GEditor->PlayWorld,false);
   auto* PC=It->GetOwningPlayer(); APawn* PreviousPawn=PC->GetPawn();
   PC->UnPossess(); It->Bind(); TestFalse(TEXT("Unpossess disconnects HUD"),It->bReady);
   PC->Possess(PreviousPawn); It->Bind(); TestTrue(TEXT("Possess reconnects HUD"),It->bReady);
   TestEqual(TEXT("Rebind does not emit fake pickup"),It->GroupWood,0);
   It->Quota->InitializeQuota(5); It->Cargo->TryAddWood(5);
   bool bStarted=false;
   for(TActorIterator<AChopItDeliveryZone> Zone(GEditor->PlayWorld);Zone;++Zone)
   {
    PreviousPawn->SetActorLocation(Zone->GetDeliverySphere()->GetComponentLocation()+FVector(0,0,80),false,nullptr,ETeleportType::TeleportPhysics);
    bStarted=Zone->Interact_Implementation(PreviousPawn); if(bStarted) break;
   }
   TestTrue(TEXT("Actual delivery interaction accepted"),bStarted);
  }
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]()
 {
  for(TObjectIterator<UChopItJuiceWidget> It;It;++It) if(It->GetWorld()==GEditor->PlayWorld && It->IsInViewport())
  {
   TestEqual(TEXT("Only actual accepted delivery emits travel units"),It->ConfirmedDeliveryUnits,5);
   TestEqual(TEXT("Actual delivery completes quota"),It->Progress,5);
  }
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("UIJuice/PIE_delivery.png"),true,false);
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.2f));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
 return true;
}
#endif
