#include "UI/ChopItJuiceWidget.h"
#include "UI/ChopItUIJuiceProfile.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Combat/ChopItHealthComponent.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "Economy/ChopItQuotaComponent.h"
#include "Economy/ChopItEconomyComponent.h"
#include "Progression/ChopItExperienceComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "HAL/PlatformMisc.h"
#include "Engine/Engine.h"
#include "Cycle/ChopItRunStateComponent.h"

UWidget* UChopItJuiceWidget::Find(FName Name) const { return WidgetTree ? WidgetTree->FindWidget(Name) : nullptr; }
void UChopItJuiceWidget::SetText(FName Name,const FString& Value) { if(auto* W=Cast<UTextBlock>(Find(Name))) W->SetText(FText::FromString(Value)); }

void UChopItJuiceWidget::NativeConstruct()
{
 Super::NativeConstruct();
 if(!JuiceProfile) JuiceProfile=LoadObject<UChopItUIJuiceProfile>(nullptr,TEXT("/Game/ChopIt/UI/DA_UIJuice.DA_UIJuice"));
 if(!JuiceProfile) JuiceProfile=NewObject<UChopItUIJuiceProfile>(this);
 TArray<UWidget*> Widgets; WidgetTree->GetAllWidgets(Widgets);
 for(auto* W:Widgets)
 {
  FVisual V; V.Transform=W->GetRenderTransform(); V.Opacity=W->GetRenderOpacity();
  if(auto* S=Cast<UCanvasPanelSlot>(W->Slot)) V.Size=S->GetSize();
  if(auto* I=Cast<UImage>(W)){ V.Brush=I->GetBrush(); V.Tint=I->GetColorAndOpacity(); }
  Originals.Add(W->GetFName(),V);
 }
 if(auto* Root=Cast<UCanvasPanel>(WidgetTree->RootWidget))
 {
  Effects=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("JuiceEffects"));
  auto* S=Root->AddChildToCanvas(Effects); S->SetAnchors(FAnchors(0,0,1,1)); S->SetOffsets(FMargin(0)); S->SetZOrder(100);
  Effects->SetVisibility(ESlateVisibility::HitTestInvisible);
  for(auto Name:{FName(TEXT("QuotaFill")),FName(TEXT("XPFill"))})
  {
   auto* Glow=WidgetTree->ConstructWidget<UImage>(); Glow->SetColorAndOpacity(JuiceProfile->Gold); auto* GlowSlot=Effects->AddChildToCanvas(Glow); GlowSlot->SetSize(FVector2D(4,24)); Glow->SetRenderOpacity(0);
   if(Name==TEXT("QuotaFill")) QuotaGlow=Glow; else XPGlow=Glow;
  }
  if(auto* I=Cast<UImage>(Find(TEXT("HealthFill"))))
  {
   DamageTrail=WidgetTree->ConstructWidget<UImage>(); DamageTrail->SetBrush(I->GetBrush()); DamageTrail->SetColorAndOpacity(FLinearColor(1,.7f,.3f,1));
   auto* TrailSlot=Root->AddChildToCanvas(DamageTrail); auto* Source=CastChecked<UCanvasPanelSlot>(I->Slot);
   TrailSlot->SetLayout(Source->GetLayout()); TrailSlot->SetZOrder(Source->GetZOrder()-1);
   Originals.Add(DamageTrail->GetFName(),Originals.FindChecked(TEXT("HealthFill")));
  }
 }
 Bind();
#if !UE_BUILD_SHIPPING
 bDemo=FParse::Param(FCommandLine::Get(),TEXT("UIJuiceDemo"));
 if(FParse::Param(FCommandLine::Get(),TEXT("UIJuiceReduced"))){ JuiceProfile=DuplicateObject<UChopItUIJuiceProfile>(JuiceProfile,this); JuiceProfile->bReducedMotion=true; }
 if(bDemo && GEngine) GEngine->bEnableOnScreenDebugMessages=false;
 if(bDemo){ Unbind(); bReady=true; HealthValue=100; HealthMax=100; Target=200; Progress=0; Balance=125; Wood=0; RevealMission(); }
#endif
}
void UChopItJuiceWidget::Unbind()
{
 if(Cargo.IsValid()){ Cargo->OnCargoChanged.RemoveDynamic(this,&ThisClass::CargoChanged); Cargo->OnPickupRejected.RemoveDynamic(this,&ThisClass::Rejected); }
 if(Quota.IsValid()){ Quota->OnQuotaChanged.RemoveDynamic(this,&ThisClass::QuotaChanged); Quota->OnDeliveryConfirmed.RemoveDynamic(this,&ThisClass::DeliveryConfirmed); }
 if(Economy.IsValid()) Economy->OnBalanceChanged.RemoveDynamic(this,&ThisClass::BalanceChanged);
 if(Experience.IsValid()) Experience->OnExperienceChanged.RemoveDynamic(this,&ThisClass::XPChanged);
 if(Health.IsValid()) Health->OnHealthChanged.RemoveAll(this);
 if(Cycle.IsValid()){ Cycle->OnClockChanged.RemoveDynamic(this,&ThisClass::ClockChanged); Cycle->OnPhaseChanged.RemoveDynamic(this,&ThisClass::PhaseChanged); }
 Cargo.Reset(); Quota.Reset(); Economy.Reset(); Experience.Reset(); Health.Reset(); Cycle.Reset(); bReady=false;
 Deferred.Reset(); GroupWood=0; GroupQuota=0; GroupMoney=0; GroupXP=0; GroupAge=0;
 DamageAge=10; XPTransition=-1; PriorityUntil=0; Punches.Reset();
 for(auto& P:Particles) if(P.Widget.IsValid()) P.Widget->RemoveFromParent();
 Particles.Reset();
 StopAllAnimations();
 for(auto& Entry:Originals) if(auto* W=Find(Entry.Key)) W->SetRenderTransform(Entry.Value.Transform);
}
void UChopItJuiceWidget::NativeDestruct()
{
 Unbind();
 for(auto& Entry:Originals) if(auto* W=Find(Entry.Key))
 {
  W->SetRenderTransform(Entry.Value.Transform); W->SetRenderOpacity(Entry.Value.Opacity);
  if(auto* S=Cast<UCanvasPanelSlot>(W->Slot)) S->SetSize(Entry.Value.Size);
  if(auto* I=Cast<UImage>(W)){ I->SetBrush(Entry.Value.Brush); I->SetColorAndOpacity(Entry.Value.Tint); }
 }
 if(Effects) Effects->RemoveFromParent();
 if(DamageTrail) DamageTrail->RemoveFromParent();
 Effects=nullptr; DamageTrail=nullptr; Stamp=nullptr; QuotaGlow=nullptr; XPGlow=nullptr;
 Originals.Reset(); Fills.Reset();
 Super::NativeDestruct();
}
void UChopItJuiceWidget::Bind()
{
 auto* PC=GetOwningPlayer(); APawn* Pawn=PC?PC->GetPawn():nullptr; auto* GS=GetWorld()?GetWorld()->GetGameState():nullptr;
 if(!Pawn || !GS || !PC->PlayerState){ Unbind(); BoundPawn.Reset(); return; }
 Unbind(); BoundPawn=Pawn;
 Cargo=Pawn->FindComponentByClass<UChopItWoodCargoComponent>(); Health=Pawn->FindComponentByClass<UChopItHealthComponent>();
 Quota=GS->FindComponentByClass<UChopItQuotaComponent>(); Cycle=GS->FindComponentByClass<UChopItCycleStateMachineComponent>();
 Economy=PC->PlayerState->FindComponentByClass<UChopItEconomyComponent>(); Experience=PC->PlayerState->FindComponentByClass<UChopItExperienceComponent>();
 if(Cargo.IsValid()){ CargoChanged(Cargo->GetCurrentWood(),Cargo->GetCapacity()); Cargo->OnCargoChanged.AddDynamic(this,&ThisClass::CargoChanged); Cargo->OnPickupRejected.AddDynamic(this,&ThisClass::Rejected); }
 if(Health.IsValid()){ HealthChanged(Health->GetCurrentHealth(),Health->GetMaxHealth(),nullptr); Health->OnHealthChanged.AddUObject(this,&ThisClass::HealthChanged); }
 if(Quota.IsValid()){ QuotaChanged(Quota->GetProgress(),Quota->GetTarget(),Quota->IsComplete()); Quota->OnQuotaChanged.AddDynamic(this,&ThisClass::QuotaChanged); Quota->OnDeliveryConfirmed.AddDynamic(this,&ThisClass::DeliveryConfirmed); }
 if(Economy.IsValid()){ BalanceChanged(Economy->GetBalance(),0); Economy->OnBalanceChanged.AddDynamic(this,&ThisClass::BalanceChanged); }
 if(Experience.IsValid()){ XPChanged(Experience->GetLevel(),Experience->GetCurrentExperience(),Experience->GetRequiredExperience(),0); Experience->OnExperienceChanged.AddDynamic(this,&ThisClass::XPChanged); }
 if(Cycle.IsValid()){ LastSecond=-1; ClockChanged(Cycle->GetCurrentPhase(),Cycle->GetPhaseRemaining()); Cycle->OnClockChanged.AddDynamic(this,&ThisClass::ClockChanged); Cycle->OnPhaseChanged.AddDynamic(this,&ThisClass::PhaseChanged); }
 bReady=true;
 if(bMissionRequested) RevealMission();
}
FVector2D UChopItJuiceWidget::Center(FName Name) const
{
 if(auto* W=Find(Name)) return GetCachedGeometry().AbsoluteToLocal(W->GetCachedGeometry().LocalToAbsolute(W->GetCachedGeometry().GetLocalSize()*.5f));
 return FVector2D::ZeroVector;
}
void UChopItJuiceWidget::Punch(FName Name,float Strength)
{
 if(!Find(Name)) return;
 if(Name==TEXT("CargoIcon") && JuicePickup && !JuiceProfile->bReducedMotion){ StopAnimation(JuicePickup); PlayAnimation(JuicePickup,0,1,EUMGSequencePlayMode::Forward,1,true); return; }
 Punches.RemoveAll([Name](const FPunch& P){return P.Widget==Name;});
 FPunch P; P.Widget=Name; P.Duration=FMath::Max(.01f,JuiceProfile->PunchDuration); P.Strength=Strength; Punches.Add(P);
}
void UChopItJuiceWidget::Sound(FName Name)
{
 float Interval=Name==TEXT("Pickup")?FMath::Max(.08f,JuiceProfile->PickupSoundInterval):.15f;
 if(const float* Previous=LastSound.Find(Name); Previous && Time-*Previous<Interval) return;
 LastSound.Add(Name,Time);
 if(auto* S=JuiceProfile->Sounds.Find(Name); S && *S){ const float Variation=FMath::Clamp(JuiceProfile->PitchVariation,0.f,.05f); UGameplayStatics::PlaySound2D(this,*S,FMath::Clamp(JuiceProfile->Volume,0.f,1.f),FMath::FRandRange(1-Variation,1+Variation)); }
}
void UChopItJuiceWidget::Float(FName Anchor,const FString& Value,FLinearColor Color,bool bDown)
{
 if(!bDispatchingDeferred && Time<PriorityUntil){ if(Deferred.Num()<16) Deferred.Add({PriorityUntil,[this,Anchor,Value,Color,bDown](){Float(Anchor,Value,Color,bDown);}}); return; }
 if(!Effects) return;
 int32 Count=0; for(const auto& P:Particles) Count+=P.bText?1:0;
 if(Count>=FMath::Clamp(JuiceProfile->MaxFloatingTexts,0,8)) return;
 auto* T=WidgetTree->ConstructWidget<UTextBlock>(); T->SetText(FText::FromString(Value)); T->SetColorAndOpacity(Color);
 auto Font=T->GetFont(); Font.Size=22; T->SetFont(Font); T->SetShadowOffset(FVector2D(2)); T->SetShadowColorAndOpacity(FLinearColor::Black);
 auto* S=Effects->AddChildToCanvas(T); S->SetAutoSize(true); S->SetAlignment(FVector2D(.5f));
 FParticle P; P.Widget=T; P.Start=Center(Anchor);
 if(Anchor==TEXT("QuotaLog")) P.Start+=FVector2D(Originals.FindChecked(Anchor).Size.X*.5f+45,15);
 if(Anchor==TEXT("MissionTitle")) P.Start=Center(TEXT("MissionFrame"))+FVector2D(0,Originals.FindChecked(TEXT("MissionFrame")).Size.Y*.5f+25);
 P.End=P.Start+(JuiceProfile->bReducedMotion?FVector2D(0,bDown?8:-8):FVector2D(10,bDown?45:-35)); P.Duration=FMath::Max(.01f,JuiceProfile->FloatingTextDuration); P.bText=true; S->SetPosition(P.Start); Particles.Add(P);
}
void UChopItJuiceWidget::Burst(FName Anchor,int32 Count,FName Sprite,FName Destination)
{
 if(!bDispatchingDeferred && Time<PriorityUntil){ if(Deferred.Num()<16) Deferred.Add({PriorityUntil,[this,Anchor,Count,Sprite,Destination](){Burst(Anchor,Count,Sprite,Destination);}}); return; }
 if(!Effects || JuiceProfile->bReducedMotion) return;
 int32 Existing=0; for(const auto& P:Particles) Existing+=P.bText?0:1;
 Count=FMath::Min(Count,FMath::Clamp(JuiceProfile->MaxParticles,0,48)-Existing);
 for(int32 N=0;N<Count;++N)
 {
  auto* I=WidgetTree->ConstructWidget<UImage>(); FVector2D Size(6,3);
  if(auto* Source=Cast<UImage>(Find(Sprite))) { I->SetBrush(Source->GetBrush()); Size=Sprite==TEXT("MoneyIcon")?FVector2D(28,16):FVector2D(28,10); }
  else { I->SetColorAndOpacity(Sprite==TEXT("Heal")?JuiceProfile->Heal:Sprite==TEXT("Smoke")?FLinearColor(.15f,.12f,.1f,.4f):JuiceProfile->Gold); if(Sprite==TEXT("Smoke")) Size=FVector2D(12,12); }
  auto* S=Effects->AddChildToCanvas(I); S->SetSize(Size); S->SetAlignment(FVector2D(.5f));
  FParticle P; P.Widget=I; P.Start=Center(Anchor); P.bTravel=!Destination.IsNone(); P.Duration=P.bTravel?FMath::Max(.01f,JuiceProfile->TravelDuration):FMath::FRandRange(.4f,.8f);
  P.End=P.bTravel?Center(Destination):P.Start+FVector2D(FMath::FRandRange(-90.f,90.f),FMath::FRandRange(-75.f,40.f));
  if(P.bTravel && Sprite==TEXT("MoneyIcon")) P.Start+=FVector2D(FMath::FRandRange(-110.f,110.f),FMath::FRandRange(-80.f,80.f));
  S->SetPosition(P.Start); Particles.Add(P);
 }
}
void UChopItJuiceWidget::SetFill(FName Name,float Fraction)
{
 auto* I=Cast<UImage>(Find(Name)); auto* V=Originals.Find(Name); if(!I || !V) return;
 const float F=FMath::Clamp(Fraction,0.f,1.f); if(auto* S=Cast<UCanvasPanelSlot>(I->Slot)) S->SetSize(FVector2D(V->Size.X*F,V->Size.Y));
 FSlateBrush Brush=V->Brush; FBox2f UV=Brush.GetUVRegion(); UV.Max.X=FMath::Lerp(UV.Min.X,UV.Max.X,F); Brush.SetUVRegion(FBox2d(UV)); I->SetBrush(Brush);
 I->SetRenderOpacity(F>0?V->Opacity:0);
}
void UChopItJuiceWidget::AnimateFill(FName Name,float Fraction,bool bImmediate)
{
 auto& F=Fills.FindOrAdd(Name); F.From=FMath::Lerp(F.From,F.To,FMath::Clamp(F.Age/FMath::Max(.001f,F.Duration),0.f,1.f)); F.To=Fraction; F.Age=0; F.Duration=FMath::Max(.01f,JuiceProfile->FillDuration);
 if(bImmediate){ F.From=Fraction; F.Age=F.Duration; SetFill(Name,Fraction); }
}
void UChopItJuiceWidget::CargoChanged(int32 Current,int32 Capacity)
{
 if(bReady && Current>Wood){ GroupWood+=Current-Wood; Punch(TEXT("CargoIcon")); Sound(TEXT("Pickup")); OnJuiceEvent(TEXT("Pickup")); }
 if(bReady && Current>=Capacity && Wood<Capacity){ Float(TEXT("CargoIcon"),TEXT("FULL"),JuiceProfile->Gold); Punch(TEXT("CargoFrame")); }
 Wood=Current; SetText(TEXT("WoodText"),FString::Printf(TEXT("%d / %d"),Current,Capacity));
 if(auto* I=Cast<UImage>(Find(TEXT("CargoIcon")))) I->SetColorAndOpacity(Current>=Capacity*.8f?JuiceProfile->Gold:Originals.FindChecked(TEXT("CargoIcon")).Tint);
}
void UChopItJuiceWidget::Rejected(int32 Units){ Punch(TEXT("CargoIcon"),1.5f); Float(TEXT("CargoIcon"),TEXT("FULL"),JuiceProfile->Danger); Sound(TEXT("Reject")); }
void UChopItJuiceWidget::DeliveryConfirmed(int32 Units){ ConfirmedDeliveryUnits+=Units; Burst(TEXT("CargoIcon"),FMath::Min(Units,6),TEXT("QuotaLog"),TEXT("QuotaLog")); Punch(TEXT("CargoIcon"),.5f); }
void UChopItJuiceWidget::HealthChanged(float Current,float Maximum,AActor*)
{
 const float Fraction=Current/FMath::Max(1.f,Maximum);
 if(bReady && Current<HealthValue){ TrailFrom=HealthValue/FMath::Max(1.f,HealthMax); DamageAge=0; PriorityUntil=Time+.4f; Punch(TEXT("HeartIcon"),1.4f); Sound(TEXT("Damage")); OnJuiceEvent(TEXT("Damage")); }
 if(bReady && Current>HealthValue){ Float(TEXT("HeartIcon"),FString::Printf(TEXT("+%.0f"),Current-HealthValue),JuiceProfile->Heal); Burst(TEXT("HeartIcon"),12,TEXT("Heal")); Punch(TEXT("HeartIcon")); Sound(TEXT("Heal")); }
 HealthValue=Current; HealthMax=Maximum; AnimateFill(TEXT("HealthFill"),Fraction,!bReady); SetText(TEXT("HealthText"),FString::Printf(TEXT("%.0f / %.0f"),Current,Maximum));
}
void UChopItJuiceWidget::QuotaChanged(int32 Current,int32 Required,bool bComplete)
{
 if(bReady && (Required!=Target || Current<Progress)){ CompleteAge=-1; if(Stamp) Stamp->RemoveFromParent(); Stamp=nullptr; RevealMission(); }
 if(bReady && Current>Progress){ Punch(TEXT("QuotaLog")); Punch(TEXT("MissionDescription"),.5f); GroupQuota+=Current-Progress; Sound(TEXT("Delivery")); }
 if(bReady && bComplete && Progress<Required)
 {
  CompleteAge=0; Burst(TEXT("QuotaLog"),24); Punch(TEXT("QuotaLog"),1.5f); Sound(TEXT("Complete")); OnJuiceEvent(TEXT("Complete"));
  Burst(TEXT("QuotaLog"),8,TEXT("Smoke")); Float(TEXT("MissionTitle"),TEXT("✓ COMPLETE"),JuiceProfile->Gold,true);
  if(JuiceStamp && !JuiceProfile->bReducedMotion) PlayAnimation(JuiceStamp,0,1,EUMGSequencePlayMode::Forward,1,true);
  if(Effects){ Stamp=WidgetTree->ConstructWidget<UTextBlock>(); Stamp->SetText(FText::FromString(TEXT("COMPLETE"))); Stamp->SetColorAndOpacity(JuiceProfile->Gold); auto Font=Stamp->GetFont(); Font.Size=22; Stamp->SetFont(Font); auto* S=Effects->AddChildToCanvas(Stamp); S->SetAutoSize(true); S->SetAlignment(FVector2D(.5f)); S->SetPosition(Center(TEXT("QuotaLog"))); }
 }
 Progress=Current; Target=Required; AnimateFill(TEXT("QuotaFill"),float(Current)/FMath::Max(1,Required),!bReady); SetText(TEXT("QuotaText"),FString::Printf(TEXT("QUOTA %d / %d"),Current,Required));
}
void UChopItJuiceWidget::BalanceChanged(int64 Current,int64 Delta)
{
 MoneyFrom=bReady?FMath::Lerp(MoneyFrom,MoneyTo,FMath::Clamp(MoneyAge/FMath::Max(.01f,JuiceProfile->MoneyDuration),0.f,1.f)):float(Current); MoneyTo=float(Current); MoneyAge=bReady?0:JuiceProfile->MoneyDuration;
 if(bReady && Delta!=0){ if(Delta>0) GroupMoney+=Delta; else Float(TEXT("MoneyIcon"),FString::Printf(TEXT("−$%lld"),FMath::Abs(Delta)),JuiceProfile->Danger,true); Punch(TEXT("MoneyIcon"),Delta>0?1.f:-1.f); Sound(Delta>0?TEXT("Money"):TEXT("Spend")); if(Delta>0) Burst(TEXT("MoneyIcon"),10,TEXT("MoneyIcon"),TEXT("MoneyIcon")); }
 Balance=Current;
}
void UChopItJuiceWidget::XPChanged(int32 NewLevel,int32 Current,int32 Required,int32 Pending)
{
 if(bReady && NewLevel>Level){ Punch(TEXT("LevelFrame"),1.6f); Burst(TEXT("LevelFrame"),24); Float(TEXT("LevelText"),FString::Printf(TEXT("LEVEL %d"),NewLevel),JuiceProfile->Gold); Sound(TEXT("Level")); OnJuiceEvent(TEXT("Level")); }
 else if(bReady){ Punch(TEXT("XPText"),.4f); }
 if(bReady && Experience.IsValid()) GroupXP+=FMath::Max(int64(0),Experience->GetTotalExperience()-TotalXP);
 if(bReady && NewLevel>Level){ XPTransition=0; XPDisplayLevel=Level; XPTargetLevel=NewLevel; XPNext=float(Current)/FMath::Max(1,Required); AnimateFill(TEXT("XPFill"),1); }
 else if(XPTransition<0) AnimateFill(TEXT("XPFill"),float(Current)/FMath::Max(1,Required),!bReady);
 else XPNext=float(Current)/FMath::Max(1,Required);
 Level=NewLevel; TotalXP=Experience.IsValid()?Experience->GetTotalExperience():TotalXP;
 SetText(TEXT("XPText"),FString::Printf(TEXT("XP %d / %d"),Current,Required)); SetText(TEXT("LevelText"),FString::Printf(TEXT("LEVEL %d"),XPTransition>=0?XPDisplayLevel:NewLevel));
}
void UChopItJuiceWidget::UpdateClockIcon(EChopItCyclePhase Phase,bool bImmediate)
{
 auto* Icon=Cast<UImage>(Find(TEXT("SunIcon"))); const auto* Original=Originals.Find(TEXT("SunIcon"));
 if(!Icon || !Original) return;
 // Non-clock phases retain the last valid icon, rather than flashing back to daylight.
 int32 Index=Phase==EChopItCyclePhase::Day?0:Phase==EChopItCyclePhase::Dusk?1:Phase==EChopItCyclePhase::Night?2:INDEX_NONE;
 if(Index==INDEX_NONE || (Index==ClockIconPhase && !bImmediate)) return;
 const bool bFirst=ClockIconPhase==INDEX_NONE;
 ClockIconPhase=Index; ClockIconNextBrush=Original->Brush;
 if(Index!=0)
 {
  // Padded atlas windows retain breathing room around each icon in the original UMG slot.
  const FVector2d Min=Index==1?FVector2d(1091,691):FVector2d(1410,691);
  const FVector2d Max=Min+FVector2d(186,177);
  ClockIconNextBrush.SetUVRegion(FBox2d(Min/FVector2d(1774,887),Max/FVector2d(1774,887)));
 }
 ClockIconFadeAge=0; bClockIconSwapped=bImmediate || bFirst;
 if(bClockIconSwapped){ Icon->SetBrush(ClockIconNextBrush); Icon->SetRenderOpacity(Original->Opacity); ClockIconFadeAge=JuiceProfile->ClockIconFadeDuration; }
}

void UChopItJuiceWidget::TickClockIcon(float DeltaTime)
{
 auto* Icon=Cast<UImage>(Find(TEXT("SunIcon"))); const auto* Original=Originals.Find(TEXT("SunIcon"));
 if(!Icon || !Original) return;
 ClockIconFadeAge+=DeltaTime;
 const float Fade=FMath::Clamp(ClockIconFadeAge/FMath::Max(.01f,JuiceProfile->ClockIconFadeDuration),0.f,1.f);
 if(!bClockIconSwapped && Fade>=.5f){ Icon->SetBrush(ClockIconNextBrush); bClockIconSwapped=true; }
 Icon->SetRenderOpacity(Original->Opacity*FMath::Abs(2*Fade-1));
 auto Transform=Original->Transform;
 if(!JuiceProfile->bReducedMotion)
 {
  Transform.Angle+=JuiceProfile->ClockIconSwingDegrees*FMath::Sin(Time*2*PI/FMath::Max(.1f,JuiceProfile->ClockIconSwingPeriod));
  if(ClockStage>0)
  {
   const float Speed=ClockStage==2?2.f:1.2f;
   Transform.Scale*=1+FMath::Clamp(JuiceProfile->ClockIconUrgentPulse,0.f,.5f)*FMath::Sin(Time*2*PI*Speed);
  }
 }
 Icon->SetRenderTransform(Transform);
}

void UChopItJuiceWidget::PhaseChanged(EChopItCyclePhase NewPhase,EChopItCyclePhase Previous,int32 Generation){ LastSecond=-1; ClockStage=0; UpdateClockIcon(NewPhase,!bReady); if(auto* T=Cast<UTextBlock>(Find(TEXT("TimeText")))) T->SetColorAndOpacity(FLinearColor::White); }
void UChopItJuiceWidget::ClockChanged(EChopItCyclePhase Phase,float Remaining)
{
 UpdateClockIcon(Phase,!bReady);
 int32 Seconds=FMath::Max(0,FMath::CeilToInt(Remaining)); SetText(TEXT("TimeText"),FString::Printf(TEXT("%02d:%02d"),Seconds/60,Seconds%60));
 const bool Urgent=Phase==EChopItCyclePhase::Day || Phase==EChopItCyclePhase::Dusk;
 float Duration=Cycle.IsValid()?(Phase==EChopItCyclePhase::Day?Cycle->GetTimings().DayDuration:Cycle->GetTimings().DuskHardDeadline):60.f;
 float Fraction=Remaining/FMath::Max(.01f,Duration); ClockStage=Urgent?(Fraction<=.1f?2:(Fraction<=.25f?1:0)):0;
 if(auto* T=Cast<UTextBlock>(Find(TEXT("TimeText")))) T->SetColorAndOpacity(ClockStage==2?JuiceProfile->Danger:ClockStage==1?JuiceProfile->Gold:FLinearColor::White);
 if(bReady && Urgent && Seconds!=LastSecond && LastSecond>=0 && ClockStage>0){ if(ClockStage==2 || Seconds%2==0) Sound(TEXT("Tick")); Punch(TEXT("TimeText"),Seconds<=5?1.4f:.35f); if(Seconds<=5) Punch(TEXT("ClockFrame")); if(Seconds==0){ PriorityUntil=Time+.4f; Sound(TEXT("Expire")); OnJuiceEvent(TEXT("Expire")); } }
 LastSecond=Seconds;
}
void UChopItJuiceWidget::RevealMission(){ bMissionRequested=true; bMissionVisible=true; MissionAge=0; if(bReady){ Sound(TEXT("Mission")); OnJuiceEvent(TEXT("Mission")); if(JuiceMission && !JuiceProfile->bReducedMotion) PlayAnimation(JuiceMission,0,1,EUMGSequencePlayMode::Forward,1,true); } }
void UChopItJuiceWidget::NativeTick(const FGeometry& Geometry,float Dt)
{
 Super::NativeTick(Geometry,Dt); if(GetWorld()->IsPaused()) return; Time+=Dt;
 if(!bDemo && (!bReady || (GetOwningPlayer() && BoundPawn.Get()!=GetOwningPlayer()->GetPawn()))) Bind();
 if(!JuiceProfile) return;
 for(int32 N=0;N<Deferred.Num();) if(Time>=Deferred[N].At){ auto Action=MoveTemp(Deferred[N].Action); Deferred.RemoveAt(N); bDispatchingDeferred=true; Action(); bDispatchingDeferred=false; } else ++N;
 if(XPTransition>=0){ XPTransition+=Dt; if(XPTransition>=JuiceProfile->FillDuration+.12f){ XPDisplayLevel++; SetText(TEXT("LevelText"),FString::Printf(TEXT("LEVEL %d"),XPDisplayLevel)); AnimateFill(TEXT("XPFill"),0,true); if(XPDisplayLevel<XPTargetLevel){ AnimateFill(TEXT("XPFill"),1); XPTransition=0; } else {AnimateFill(TEXT("XPFill"),XPNext); XPTransition=-1;} } }
 if(!bDemo && GetWorld()->GetGameState()) if(auto* Run=GetWorld()->GetGameState()->FindComponentByClass<UChopItRunStateComponent>()) SetText(TEXT("DayText"),FString::Printf(TEXT("DAY %d"),Run->GetDayNumber()));
#if !UE_BUILD_SHIPPING
 if(bDemo)
 {
  static const FName Events[]={TEXT("Mission"),TEXT("Pickup"),TEXT("Reject"),TEXT("Delivery"),TEXT("Damage"),TEXT("Heal"),TEXT("Money"),TEXT("Spend"),TEXT("Level"),TEXT("Critical"),TEXT("Clock"),TEXT("Complete"),TEXT("Expire"),TEXT("ResetPhase"),TEXT("Bars0"),TEXT("Bars25"),TEXT("Bars50"),TEXT("Bars100"),TEXT("MultiLevel"),TEXT("Burst"),TEXT("Sunset"),TEXT("DayReturn")};
  const int32 Step=FMath::FloorToInt((Time-2)/2);
  if(Step>=0 && Step<UE_ARRAY_COUNT(Events) && Step!=DemoStep){ DemoStep=Step; PreviewJuiceEvent(Events[Step]); const bool bIconEvent=Events[Step]==TEXT("Sunset") || Events[Step]==TEXT("DayReturn") || Events[Step]==TEXT("ResetPhase"); DemoCaptureAt=Time+(bIconEvent?.65f:.15f); UE_LOG(LogTemp,Display,TEXT("UI_JUICE_DEMO %s"),*Events[Step].ToString()); }
  if(DemoCaptureAt>0 && Time>=DemoCaptureAt){ FString Run=FString::Printf(TEXT("%dx%d_%s"),GSystemResolution.ResX,GSystemResolution.ResY,JuiceProfile->bReducedMotion?TEXT("Reduced"):TEXT("Arcade")); FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("UIJuice")/Run/FString::Printf(TEXT("%02d_%s.png"),DemoStep,*Events[DemoStep].ToString()),true,false); DemoCaptureAt=-1; }
  if(Time>UE_ARRAY_COUNT(Events)*2+4) FPlatformMisc::RequestExit(false);
 }
#endif
 for(auto& Entry:Originals) if(auto* W=Find(Entry.Key))
 {
  if((Entry.Key==TEXT("CargoIcon") && JuicePickup && IsAnimationPlaying(JuicePickup)) || (Entry.Key==TEXT("MissionFrame") && JuiceMission && IsAnimationPlaying(JuiceMission)) || (Entry.Key==TEXT("QuotaTextContainer") && JuiceStamp && IsAnimationPlaying(JuiceStamp))) continue;
  W->SetRenderTransform(Entry.Value.Transform);
 }
 for(int32 N=Punches.Num()-1;N>=0;--N)
 {
  auto& P=Punches[N]; P.Age+=Dt; float A=FMath::Clamp(P.Age/P.Duration,0.f,1.f); float E=JuiceProfile->Envelope?JuiceProfile->Envelope->GetFloatValue(A):FMath::Sin(A*PI)*(1-A);
  if(auto* W=Find(P.Widget)) if(auto* V=Originals.Find(P.Widget)){ auto T=V->Transform; float Strength=JuiceProfile->bReducedMotion?.1f:P.Strength; if(P.Widget==TEXT("QuotaLog")){ T.Translation+=FVector2D(8*FMath::Sin(A*PI*6),4*FMath::Sin(A*PI*4))*(1-A)*Strength; } else { T.Scale*=1+(JuiceProfile->PunchScale-1)*E*Strength; T.Angle+=JuiceProfile->WobbleDegrees*FMath::Sin(A*PI*4)*(1-A)*Strength; } W->SetRenderTransform(T); }
  if(A>=1) Punches.RemoveAt(N);
 }
 for(auto& Pair:Fills){ auto& F=Pair.Value; F.Age+=Dt; SetFill(Pair.Key,FMath::Lerp(F.From,F.To,FMath::Clamp(F.Age/F.Duration,0.f,1.f))); }
 // The quota artwork is one visual assembly: its full and empty layers move together.
 if(auto* Log=Find(TEXT("QuotaLog"))) if(auto* Fill=Find(TEXT("QuotaFill")))
 {
  auto* Base=Originals.Find(TEXT("QuotaLog")); auto* FillBase=Originals.Find(TEXT("QuotaFill"));
  if(Base && FillBase){ auto T=FillBase->Transform; const auto L=Log->GetRenderTransform(); T.Translation+=L.Translation-Base->Transform.Translation; T.Angle+=L.Angle-Base->Transform.Angle; T.Scale*=L.Scale/Base->Transform.Scale; Fill->SetRenderTransform(T); }
 }
 for(auto Name:{FName(TEXT("QuotaFill")),FName(TEXT("XPFill"))}) if(auto* I=Cast<UImage>(Find(Name)))
 {
  auto* Glow=Name==TEXT("QuotaFill")?QuotaGlow.Get():XPGlow.Get(); auto* F=Fills.Find(Name); if(!Glow || !F) continue;
  auto* S=Cast<UCanvasPanelSlot>(Glow->Slot); const auto& G=I->GetCachedGeometry(); S->SetPosition(Geometry.AbsoluteToLocal(G.LocalToAbsolute(FVector2D(G.GetLocalSize().X,0)))); S->SetSize(FVector2D(4,G.GetLocalSize().Y)); Glow->SetRenderOpacity(F->Age<F->Duration?.8f:0);
 }
 DamageAge+=Dt;
 TickClockIcon(Dt);
 if(DamageAge<.3f && !JuiceProfile->bReducedMotion) if(auto* W=Find(TEXT("HeartIcon"))){ auto T=W->GetRenderTransform(); T.Scale*=DamageAge<.1f?.75f:FMath::Lerp(1.2f,1.f,(DamageAge-.1f)/.2f); W->SetRenderTransform(T); }
 if(ClockStage==2 && !JuiceProfile->bReducedMotion) if(auto* W=Find(TEXT("TimeText"))){ auto T=W->GetRenderTransform(); T.Scale*=1+.08f*FMath::Max(0.f,FMath::Sin(Time*(LastSecond<=5?16.f:8.f))); W->SetRenderTransform(T); }
 if(DamageTrail) SetFill(DamageTrail->GetFName(),FMath::Lerp(TrailFrom,HealthValue/FMath::Max(1.f,HealthMax),FMath::Clamp((DamageAge-JuiceProfile->DamageTrailDelay)/FMath::Max(.01f,JuiceProfile->DamageTrailDuration),0.f,1.f)));
 if(auto* I=Cast<UImage>(Find(TEXT("HealthFrame")))) I->SetColorAndOpacity(DamageAge<.18f?JuiceProfile->Danger:Originals.FindChecked(TEXT("HealthFrame")).Tint);
 if(HealthValue>0 && HealthValue/HealthMax<.25f && !JuiceProfile->bReducedMotion) if(auto* W=Find(TEXT("HeartIcon"))){ auto T=Originals.FindChecked(TEXT("HeartIcon")).Transform; T.Scale*=1+.12f*FMath::Max(0.f,FMath::Sin(Time*(10-6*HealthValue/HealthMax))); W->SetRenderTransform(T); }
 MoneyAge+=Dt; SetText(TEXT("MoneyText"),FString::Printf(TEXT("$ %lld"),MoneyAge>=JuiceProfile->MoneyDuration?Balance:int64(FMath::Lerp(MoneyFrom,MoneyTo,FMath::Clamp(MoneyAge/FMath::Max(.01f,JuiceProfile->MoneyDuration),0.f,1.f)))));
 GroupAge+=Dt; if(GroupAge>=FMath::Max(.01f,JuiceProfile->GainGroupWindow)){
  if(GroupWood>0) Float(TEXT("CargoIcon"),FString::Printf(TEXT("+%d"),GroupWood),JuiceProfile->Gold);
  if(GroupQuota>0) Float(TEXT("QuotaLog"),FString::Printf(TEXT("+%d"),GroupQuota),JuiceProfile->Gold);
  if(GroupMoney>0) Float(TEXT("MoneyIcon"),FString::Printf(TEXT("+$%lld"),GroupMoney),JuiceProfile->Gold);
  if(GroupXP>0) Float(TEXT("XPText"),FString::Printf(TEXT("+%lld XP"),GroupXP),JuiceProfile->Heal);
  GroupWood=0; GroupQuota=0; GroupMoney=0; GroupXP=0; GroupAge=0;
 }
 MissionAge+=Dt; if(CompleteAge>=0) CompleteAge+=Dt;
 for(FName Name:{FName(TEXT("MissionFrame")),FName(TEXT("MissionTitle")),FName(TEXT("MissionDescription"))}) if(auto* W=Find(Name))
 {
  auto* V=Originals.Find(Name); if(!V) continue; float Delay=Name==TEXT("MissionTitle")?.1f:Name==TEXT("MissionDescription")?.2f:0;
  float A=FMath::Clamp((MissionAge-Delay)/FMath::Max(.01f,JuiceProfile->MissionDuration),0.f,1.f); W->SetRenderOpacity(bMissionVisible && (CompleteAge<0 || CompleteAge<2)?V->Opacity*A:0);
  if(!JuiceProfile->bReducedMotion){ auto T=W->GetRenderTransform(); T.Translation.Y-=60*(1-A); T.Angle+=8*FMath::Sin(A*PI*3)*(1-A); W->SetRenderTransform(T); }
 }
 if(Stamp){ Stamp->SetColorAndOpacity(FMath::Lerp(JuiceProfile->Gold,FLinearColor(.2f,.1f,.04f,1),FMath::Clamp(CompleteAge,0.f,1.f))); Stamp->SetRenderScale(FVector2D(JuiceProfile->bReducedMotion?1:1+FMath::Max(0.f,.3f-CompleteAge))); }
 for(int32 N=Particles.Num()-1;N>=0;--N){ auto& P=Particles[N]; P.Age+=Dt; float A=FMath::Clamp(P.Age/P.Duration,0.f,1.f); if(auto* W=P.Widget.Get()){ if(auto* S=Cast<UCanvasPanelSlot>(W->Slot)){ FVector2D Pos=FMath::Lerp(P.Start,P.End,A); if(P.bTravel) Pos.Y-=FMath::Sin(A*PI)*35; S->SetPosition(Pos); } W->SetRenderOpacity(1-A); if(!P.bText) W->SetRenderTransformAngle(A*100); if(A>=1) W->RemoveFromParent(); } if(A>=1) Particles.RemoveAt(N); }
}
void UChopItJuiceWidget::PreviewJuiceEvent(FName Event)
{
#if !UE_BUILD_SHIPPING
 if(Event==TEXT("Pickup")) CargoChanged(Wood+1,24);
 else if(Event==TEXT("Reject")) Rejected(1);
 else if(Event==TEXT("Damage")) HealthChanged(FMath::Max(1.f,HealthValue-20),HealthMax,nullptr);
 else if(Event==TEXT("Heal")) HealthChanged(FMath::Min(HealthMax,HealthValue+20),HealthMax,nullptr);
 else if(Event==TEXT("Money")) BalanceChanged(Balance+50,50);
 else if(Event==TEXT("Spend")) BalanceChanged(Balance-25,-25);
 else if(Event==TEXT("Complete")) QuotaChanged(FMath::Max(1,Target),FMath::Max(1,Target),true);
 else if(Event==TEXT("Mission")) RevealMission();
 else if(Event==TEXT("Level")) XPChanged(Level+1,15,100,1);
 else if(Event==TEXT("Delivery")){ QuotaChanged(50,200,false); DeliveryConfirmed(5); }
 else if(Event==TEXT("Critical")) HealthChanged(15,100,nullptr);
 else if(Event==TEXT("Clock")){ LastSecond=6; ClockChanged(EChopItCyclePhase::Day,5); }
 else if(Event==TEXT("Expire")){ LastSecond=1; ClockChanged(EChopItCyclePhase::Day,0); }
 else if(Event==TEXT("ResetPhase")){ PhaseChanged(EChopItCyclePhase::Night,EChopItCyclePhase::Dusk,2); ClockChanged(EChopItCyclePhase::Night,5); }
 else if(Event==TEXT("Sunset")){ PhaseChanged(EChopItCyclePhase::Dusk,EChopItCyclePhase::Day,3); ClockChanged(EChopItCyclePhase::Dusk,5); }
 else if(Event==TEXT("DayReturn")){ PhaseChanged(EChopItCyclePhase::Day,EChopItCyclePhase::Night,4); ClockChanged(EChopItCyclePhase::Day,60); }
 else if(Event.ToString().StartsWith(TEXT("Bars"))){ float F=FCString::Atof(*Event.ToString().Mid(4))/100.f; for(FName Name:{FName(TEXT("HealthFill")),FName(TEXT("QuotaFill")),FName(TEXT("XPFill"))}) AnimateFill(Name,F,true); HealthValue=F*100; SetText(TEXT("HealthText"),FString::Printf(TEXT("%.0f / 100"),HealthValue)); if(Stamp){Stamp->RemoveFromParent();Stamp=nullptr;} }
 else if(Event==TEXT("MultiLevel")) XPChanged(Level+3,20,100,3);
 else if(Event==TEXT("Burst")){ for(int32 N=0;N<100;++N){ CargoChanged(Wood+1,999); BalanceChanged(Balance+1,1); } HealthChanged(40,100,nullptr); }
#endif
}
