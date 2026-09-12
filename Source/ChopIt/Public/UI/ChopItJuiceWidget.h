#pragma once
#include "Blueprint/UserWidget.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "ChopItJuiceWidget.generated.h"

class UChopItUIJuiceProfile;
class UChopItWoodCargoComponent;
class UChopItQuotaComponent;
class UChopItHealthComponent;
class UChopItEconomyComponent;
class UChopItExperienceComponent;
class UCanvasPanel;
class UImage;
class UTextBlock;

UCLASS()
class CHOPIT_API UChopItJuiceWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Juice") TObjectPtr<UChopItUIJuiceProfile> JuiceProfile;
 UFUNCTION(BlueprintCallable, Category="Juice") void RevealMission();
 UFUNCTION(BlueprintCallable, Category="Juice", meta=(DevelopmentOnly)) void PreviewJuiceEvent(FName Event);
 UFUNCTION(BlueprintImplementableEvent, Category="Juice") void OnJuiceEvent(FName Event);
protected:
 UPROPERTY(Transient, meta=(BindWidgetAnimOptional)) TObjectPtr<UWidgetAnimation> JuicePickup;
 UPROPERTY(Transient, meta=(BindWidgetAnimOptional)) TObjectPtr<UWidgetAnimation> JuiceMission;
 UPROPERTY(Transient, meta=(BindWidgetAnimOptional)) TObjectPtr<UWidgetAnimation> JuiceStamp;
 virtual void NativeConstruct() override;
 virtual void NativeDestruct() override;
 virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
 friend class FChopItUIJuiceTest;
 friend class FChopItUIJuicePIETest;
 struct FVisual { FVector2D Size; FWidgetTransform Transform; FSlateBrush Brush; FLinearColor Tint=FLinearColor::White; float Opacity=1; };
 struct FPunch { FName Widget; float Age=0; float Duration=.45f; float Strength=1; };
 struct FParticle { TWeakObjectPtr<UWidget> Widget; FVector2D Start,End; float Age=0,Duration=.6f; bool bText=false; bool bTravel=false; };
 struct FFill { float From=0,To=0,Age=0,Duration=.25f; };
 struct FDeferred { float At; TFunction<void()> Action; };
 TArray<FDeferred> Deferred;
 bool bDispatchingDeferred=false;
 float PriorityUntil=0;
 float XPTransition=-1, XPNext=0;
 int32 XPDisplayLevel=1, XPTargetLevel=1;
 UPROPERTY(Transient) TObjectPtr<UImage> QuotaGlow;
 UPROPERTY(Transient) TObjectPtr<UImage> XPGlow;
 TMap<FName,FVisual> Originals;
 TArray<FPunch> Punches;
 TArray<FParticle> Particles;
 TMap<FName,FFill> Fills;
 TMap<FName,float> LastSound;
 TWeakObjectPtr<APawn> BoundPawn;
 TWeakObjectPtr<UChopItWoodCargoComponent> Cargo;
 TWeakObjectPtr<UChopItQuotaComponent> Quota;
 TWeakObjectPtr<UChopItHealthComponent> Health;
 TWeakObjectPtr<UChopItEconomyComponent> Economy;
 TWeakObjectPtr<UChopItExperienceComponent> Experience;
 TWeakObjectPtr<UChopItCycleStateMachineComponent> Cycle;
 UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Effects;
 UPROPERTY(Transient) TObjectPtr<UImage> DamageTrail;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Stamp;
 float Time=0, HealthValue=0, HealthMax=1, MoneyAge=1, MoneyFrom=0, MoneyTo=0;
 float DamageAge=10, TrailFrom=0, MissionAge=10, CompleteAge=-1, GroupAge=0;
 int32 Wood=0, Target=0, Progress=0, Level=1, LastSecond=-1, ClockStage=0, GroupWood=0;
 int64 Balance=0, TotalXP=0;
 int64 GroupMoney=0, GroupXP=0;
 int32 GroupQuota=0;
 int32 ConfirmedDeliveryUnits=0;
 int32 ClockIconPhase=INDEX_NONE;
 float ClockIconFadeAge=1.f;
 FSlateBrush ClockIconNextBrush;
 bool bClockIconSwapped=true;
 bool bReady=false, bMissionVisible=false, bMissionRequested=false;
 bool bDemo=false;
 int32 DemoStep=-1;
 float DemoCaptureAt=-1;
 void Bind();
 void UpdateClockIcon(EChopItCyclePhase Phase,bool bImmediate=false);
 void TickClockIcon(float DeltaTime);
 void Unbind();
 UWidget* Find(FName Name) const;
 FVector2D Center(FName Name) const;
 void Punch(FName Name,float Strength=1);
 void Sound(FName Name);
 void Burst(FName Anchor,int32 Count,FName Sprite=NAME_None,FName Destination=NAME_None);
 void Float(FName Anchor,const FString& Value,FLinearColor Color,bool bDown=false);
 void AnimateFill(FName Name,float Fraction,bool bImmediate=false);
 void SetFill(FName Name,float Fraction);
 void SetText(FName Name,const FString& Value);
 UFUNCTION() void CargoChanged(int32 Current,int32 Capacity);
 UFUNCTION() void Rejected(int32 Units);
 UFUNCTION() void DeliveryConfirmed(int32 Units);
 UFUNCTION() void QuotaChanged(int32 Current,int32 Required,bool bComplete);
 UFUNCTION() void BalanceChanged(int64 Current,int64 Delta);
 UFUNCTION() void XPChanged(int32 NewLevel,int32 Current,int32 Required,int32 Pending);
 UFUNCTION() void ClockChanged(EChopItCyclePhase Phase,float Remaining);
 UFUNCTION() void PhaseChanged(EChopItCyclePhase NewPhase,EChopItCyclePhase Previous,int32 Generation);
 void HealthChanged(float Current,float Maximum,AActor* Source);
};
