#include "WidgetBlueprint.h"
#include "Animation/WidgetAnimation.h"
#include "Animation/MovieScene2DTransformTrack.h"
#include "Animation/MovieScene2DTransformSection.h"
#include "MovieScene.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

bool InstallUIJuiceAnimations()
{
 auto* BP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/ChopIt/UI/WBP_PSX_HUD.WBP_PSX_HUD")); if(!BP) return false;
 struct FSpec { const TCHAR* Name; const TCHAR* Widget; float Duration; };
 const FSpec Specs[]={{TEXT("JuicePickup"),TEXT("CargoIcon"),.45f},{TEXT("JuiceMission"),TEXT("MissionFrame"),.6f},{TEXT("JuiceStamp"),TEXT("QuotaTextContainer"),.45f}};
 for(const auto& Spec:Specs)
 {
  if(!BP->WidgetVariableNameToGuidMap.Contains(FName(Spec.Name))) BP->WidgetVariableNameToGuidMap.Add(FName(Spec.Name),FGuid::NewGuid());
  if(BP->Animations.ContainsByPredicate([&](const UWidgetAnimation* A){return A && A->GetFName()==FName(Spec.Name);})) continue;
  auto* W=BP->WidgetTree->FindWidget(Spec.Widget); if(!W) continue;
  auto* A=NewObject<UWidgetAnimation>(BP,Spec.Name,RF_Transactional);
  A->MovieScene=NewObject<UMovieScene>(A,Spec.Name,RF_Transactional); A->MovieScene->SetTickResolutionDirectly(FFrameRate(1000,1)); A->MovieScene->SetPlaybackRange(0,FMath::RoundToInt(Spec.Duration*1000));
  const FGuid Guid=A->MovieScene->AddPossessable(Spec.Widget,W->GetClass()); FWidgetAnimationBinding Binding; Binding.WidgetName=W->GetFName(); Binding.AnimationGuid=Guid; A->AnimationBindings.Add(Binding);
  auto* Track=A->MovieScene->AddTrack<UMovieScene2DTransformTrack>(Guid); Track->SetPropertyNameAndPath(TEXT("RenderTransform"),TEXT("RenderTransform"));
  auto* Section=CastChecked<UMovieScene2DTransformSection>(Track->CreateNewSection()); Track->AddSection(*Section); Section->SetRange(TRange<FFrameNumber>(0,FMath::RoundToInt(Spec.Duration*1000))); Section->SetCompletionMode(EMovieSceneCompletionMode::RestoreState);
  const auto T=W->GetRenderTransform(); auto Channels=Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
  const float Base[]={float(T.Translation.X),float(T.Translation.Y),T.Angle,float(T.Scale.X),float(T.Scale.Y),float(T.Shear.X),float(T.Shear.Y)};
  for(int32 I=0;I<Channels.Num() && I<7;++I){ Channels[I]->SetDefault(Base[I]); Channels[I]->AddLinearKey(0,Base[I]); Channels[I]->AddLinearKey(FMath::RoundToInt(Spec.Duration*1000)-1,Base[I]); }
  if(Channels.Num()>=5){ Channels[2]->AddCubicKey(100,T.Angle+12); Channels[2]->AddCubicKey(220,T.Angle-8); Channels[3]->AddCubicKey(100,T.Scale.X*1.25f); Channels[4]->AddCubicKey(100,T.Scale.Y*1.25f); }
  BP->Animations.Add(A);
 }
 FKismetEditorUtilities::CompileBlueprint(BP); BP->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
 return UPackage::SavePackage(BP->GetOutermost(),BP,*FPackageName::LongPackageNameToFilename(TEXT("/Game/ChopIt/UI/WBP_PSX_HUD"),FPackageName::GetAssetPackageExtension()),Args);
}
