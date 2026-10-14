#pragma once

#include "UObject/Object.h"
#include "TimerManager.h"
#include "ChopItPhoneIndicatorBinding.generated.h"

class AChopItMysteryPhone;
class APlayerController;
class UChopItPhoneIndicatorConfig;
class UChopItPhoneIndicatorViewModel;
class UChopItPhoneIndicatorWidget;

struct CHOPIT_API FChopItPhoneIndicatorPlacement
{
	FVector2D Position = FVector2D::ZeroVector;
	float ArrowDegrees = 0.0f;
	bool bOffscreen = false;
};

/** Camera-relative projection kept separate for resolution and behind-camera tests. */
struct CHOPIT_API FChopItPhoneIndicatorProjection
{
	static FChopItPhoneIndicatorPlacement Resolve(FVector2D ViewportSize, FVector2D Projected,
		bool bProjected, FVector CameraLocalDirection, float Margin, FVector2D OnscreenOffset);
	static float ResolveWorldScale(float ProjectedWidth, float WidgetWidth, float ViewportScale);
};

/** Connects one phone's ring events to a cached widget through an MVVM view model. */
UCLASS()
class CHOPIT_API UChopItPhoneIndicatorBinding final : public UObject
{
	GENERATED_BODY()
public:
	void Initialize(APlayerController* InPlayer, AChopItMysteryPhone* InPhone, UChopItPhoneIndicatorConfig* InConfig);
	void Shutdown();
	void RefreshPresentation() { UpdateIndicator(); }
	AChopItMysteryPhone* GetPhone() const { return Phone.Get(); }
	UChopItPhoneIndicatorViewModel* GetViewModel() const { return ViewModel; }

private:
	UFUNCTION() void HandleRingStarted();
	UFUNCTION() void HandleRingStopped();
	UFUNCTION() void HandleRingBeat(int32 BeatIndex);
	void UpdateIndicator();
	void EnsureWidget();
	void StopUpdate();

	TWeakObjectPtr<APlayerController> Player;
	TWeakObjectPtr<AChopItMysteryPhone> Phone;
	UPROPERTY(Transient) TObjectPtr<UChopItPhoneIndicatorConfig> Config;
	UPROPERTY(Transient) TObjectPtr<UChopItPhoneIndicatorViewModel> ViewModel;
	UPROPERTY(Transient) TObjectPtr<UChopItPhoneIndicatorWidget> Widget;
	FTimerHandle UpdateTimer;
	float Opacity = 0.0f;
	float RingElapsed = 0.0f;
	float BeatElapsed = 1.0f;
	FVector2D BeatJitter = FVector2D::ZeroVector;
	float BeatTiltDegrees = 0.0f;
	bool bRingActive = false;
};
