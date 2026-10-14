#include "UI/Phone/ChopItPhoneIndicatorBinding.h"

#include "UI/Phone/ChopItPhoneIndicatorConfig.h"
#include "UI/Phone/ChopItPhoneIndicatorViewModel.h"
#include "UI/Phone/ChopItPhoneIndicatorWidget.h"
#include "UI/ChopItHUD.h"
#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneCallComponent.h"
#include "Phone/ChopItPhoneStateComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

FChopItPhoneIndicatorPlacement FChopItPhoneIndicatorProjection::Resolve(
	FVector2D ViewportSize, FVector2D Projected, const bool bProjected,
	FVector CameraLocalDirection, const float Margin, FVector2D OnscreenOffset)
{
	FChopItPhoneIndicatorPlacement Result;
	const FVector2D Center = ViewportSize * 0.5f;
	const FVector2D Half(FMath::Max(1.0f, Center.X - Margin), FMath::Max(1.0f, Center.Y - Margin));
	const FVector2D Onscreen = Projected + OnscreenOffset;
	if (bProjected && CameraLocalDirection.Z > 0.0f &&
		FMath::Abs(Onscreen.X - Center.X) <= Half.X && FMath::Abs(Onscreen.Y - Center.Y) <= Half.Y)
	{
		Result.Position = Onscreen;
		return Result;
	}
	Result.bOffscreen = true;
	FVector2D Direction = bProjected && CameraLocalDirection.Z > 0.0f
		? Projected - Center : FVector2D(CameraLocalDirection.X, -CameraLocalDirection.Y);
	if (Direction.IsNearlyZero()) Direction = FVector2D(0, 1);
	Direction.Normalize();
	const float AlongX = FMath::Abs(Direction.X) > KINDA_SMALL_NUMBER ? Half.X / FMath::Abs(Direction.X) : BIG_NUMBER;
	const float AlongY = FMath::Abs(Direction.Y) > KINDA_SMALL_NUMBER ? Half.Y / FMath::Abs(Direction.Y) : BIG_NUMBER;
	Result.Position = Center + Direction * FMath::Min(AlongX, AlongY);
	Result.ArrowDegrees = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));
	return Result;
}

float FChopItPhoneIndicatorProjection::ResolveWorldScale(
	const float ProjectedWidth, const float WidgetWidth, const float ViewportScale)
{
	return FMath::Max(0.0f, ProjectedWidth) /
		(FMath::Max(1.0f, WidgetWidth) * FMath::Max(KINDA_SMALL_NUMBER, ViewportScale));
}

void UChopItPhoneIndicatorBinding::Initialize(APlayerController* InPlayer, AChopItMysteryPhone* InPhone,
	UChopItPhoneIndicatorConfig* InConfig)
{
	Player = InPlayer;
	Phone = InPhone;
	Config = InConfig;
	if (!InPlayer || !InPhone || !InConfig || !InConfig->IsValidConfig() || !InPhone->GetPhoneCall()) return;
	ViewModel = NewObject<UChopItPhoneIndicatorViewModel>(this);
	ViewModel->SetLabel(InConfig->Label);
	InPhone->GetPhoneCall()->OnRingStarted.AddDynamic(this, &UChopItPhoneIndicatorBinding::HandleRingStarted);
	InPhone->GetPhoneCall()->OnRingStopped.AddDynamic(this, &UChopItPhoneIndicatorBinding::HandleRingStopped);
	InPhone->GetPhoneCall()->OnRingBeat.AddDynamic(this, &UChopItPhoneIndicatorBinding::HandleRingBeat);
	if (InPhone->GetPhoneState() && InPhone->GetPhoneState()->GetState() == EChopItPhoneState::Ringing)
		HandleRingStarted();
}

void UChopItPhoneIndicatorBinding::Shutdown()
{
	if (Phone.IsValid() && Phone->GetPhoneCall())
	{
		Phone->GetPhoneCall()->OnRingStarted.RemoveDynamic(this, &UChopItPhoneIndicatorBinding::HandleRingStarted);
		Phone->GetPhoneCall()->OnRingStopped.RemoveDynamic(this, &UChopItPhoneIndicatorBinding::HandleRingStopped);
		Phone->GetPhoneCall()->OnRingBeat.RemoveDynamic(this, &UChopItPhoneIndicatorBinding::HandleRingBeat);
	}
	StopUpdate();
	if (Widget)
	{
		Widget->SetViewModel(nullptr);
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
	ViewModel = nullptr;
}

void UChopItPhoneIndicatorBinding::EnsureWidget()
{
	if (Widget || !Player.IsValid() || !Config || !ViewModel) return;
	UClass* Class = Config->WidgetClass ? Config->WidgetClass.Get() : UChopItPhoneIndicatorWidget::StaticClass();
	Widget = CreateWidget<UChopItPhoneIndicatorWidget>(Player.Get(), Class);
	if (Widget)
	{
		Widget->SetArtwork(Config->BubbleTexture, Config->RingTexture);
		Widget->SetViewModel(ViewModel);
		Widget->AddToPlayerScreen(-210);
	}
}

void UChopItPhoneIndicatorBinding::HandleRingStarted()
{
	if (!Player.IsValid() || !Phone.IsValid() || !Config) return;
	EnsureWidget();
	UE_LOG(LogTemp, Display, TEXT("Phone indicator ring started: phone=%s widget=%s range=%.0f."),
		*GetNameSafe(Phone.Get()), *GetNameSafe(Widget), Phone->GetPhoneCall()->GetAudibleDistance());
	bRingActive = true;
	RingElapsed = 0.0f;
	BeatElapsed = 1.0f;
	BeatJitter = FVector2D::ZeroVector;
	BeatTiltDegrees = 0.0f;
	if (UWorld* World = Player->GetWorld())
	{
		World->GetTimerManager().SetTimer(UpdateTimer, this, &UChopItPhoneIndicatorBinding::UpdateIndicator,
			FMath::Min(Config->UpdatePeriod, 1.0f / 60.0f), true);
		UpdateIndicator();
	}
}

void UChopItPhoneIndicatorBinding::HandleRingStopped()
{
	bRingActive = false;
	BeatJitter = FVector2D::ZeroVector;
	BeatTiltDegrees = 0.0f;
}

void UChopItPhoneIndicatorBinding::HandleRingBeat(int32 BeatIndex)
{
	BeatElapsed = 0.0f;
	BeatJitter = FVector2D((FMath::RandBool() ? 1.0f : -1.0f) * FMath::FRandRange(10.0f, 18.0f),
		(FMath::RandBool() ? 1.0f : -1.0f) * FMath::FRandRange(5.0f, 11.0f));
	BeatTiltDegrees = (FMath::RandBool() ? 1.0f : -1.0f) * FMath::FRandRange(2.5f, 5.5f);
	UE_LOG(LogTemp, Verbose, TEXT("Phone indicator beat %d: offset=(%.1f, %.1f) angle=%.1f."),
		BeatIndex + 1, BeatJitter.X, BeatJitter.Y, BeatTiltDegrees);
}

void UChopItPhoneIndicatorBinding::StopUpdate()
{
	if (Player.IsValid() && Player->GetWorld()) Player->GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
	if (ViewModel) ViewModel->SetVisible(false);
	Opacity = 0.0f;
	BeatJitter = FVector2D::ZeroVector;
	BeatTiltDegrees = 0.0f;
}

void UChopItPhoneIndicatorBinding::UpdateIndicator()
{
	if (!Player.IsValid() || !Phone.IsValid() || !Config || !ViewModel || !Widget)
	{
		StopUpdate();
		return;
	}
	if (const AChopItHUD* HUD = Player->GetHUD<AChopItHUD>(); HUD && HUD->IsModalOverlayActive())
	{
		ViewModel->SetVisible(false);
		if (!bRingActive) StopUpdate();
		return;
	}
	const float Delta = FMath::Min(Config->UpdatePeriod, 1.0f / 60.0f);
	RingElapsed += bRingActive ? Delta : 0.0f;
	BeatElapsed += Delta;
	FVector CameraLocation;
	FRotator CameraRotation;
	Player->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector PhoneLocation = Phone->GetActorLocation();
	const FVector ComicLocation = PhoneLocation + FVector(0, 0, Config->WorldHeight)
		+ CameraRotation.RotateVector(FVector::RightVector) * Config->WorldRightOffset;
	FVector ListenerLocation, ListenerFront, ListenerRight;
	Player->GetAudioListenerPosition(ListenerLocation, ListenerFront, ListenerRight);
	const UAudioComponent* RingAudio = Phone->GetPhoneCall()->GetRingAudio();
	const FVector SoundLocation = RingAudio ? RingAudio->GetComponentLocation() : PhoneLocation;
	const float Distance = FVector::Dist(ListenerLocation, SoundLocation);
	const float AudibleDistance = Phone->GetPhoneCall()->GetAudibleDistance();
	if (Distance > AudibleDistance)
	{
		Opacity = 0.0f;
		ViewModel->SetVisible(false);
		return;
	}
	const bool bInRange = bRingActive;
	Opacity = FMath::Clamp(Opacity + (bInRange ? Delta / Config->EntranceDuration : -Delta / Config->ExitDuration), 0.0f, 1.0f);
	if (Opacity <= 0.0f && !bRingActive)
	{
		StopUpdate();
		return;
	}
	if (Opacity <= 0.0f)
	{
		ViewModel->SetVisible(false);
		return;
	}
	int32 Width = 0, Height = 0;
	Player->GetViewportSize(Width, Height);
	if (Width <= 0 || Height <= 0) return;
	const FVector ToPhone = ComicLocation - CameraLocation;
	const FVector LocalDirection(
		FVector::DotProduct(ToPhone, CameraRotation.RotateVector(FVector::RightVector)),
		FVector::DotProduct(ToPhone, CameraRotation.RotateVector(FVector::UpVector)),
		FVector::DotProduct(ToPhone, CameraRotation.Vector()));
	FVector2D Projected;
	const bool bProjected = Player->ProjectWorldLocationToScreen(ComicLocation, Projected, true);
	// Project a camera-facing world span at the comic's depth. This respects FOV,
	// resolution and perspective without using the listener or hearing radius.
	const FVector SpanCenter = CameraLocation + CameraRotation.Vector() * FMath::Max(1.0f, FMath::Abs(LocalDirection.Z));
	FVector2D SpanStart, SpanEnd;
	if (!Player->ProjectWorldLocationToScreen(SpanCenter, SpanStart, true) ||
		!Player->ProjectWorldLocationToScreen(SpanCenter + CameraRotation.RotateVector(FVector::RightVector) * Config->WorldWidth, SpanEnd, true)) return;
	Widget->ForceLayoutPrepass();
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(Player.Get());
	const float BaseScale = FChopItPhoneIndicatorProjection::ResolveWorldScale(
		FVector2D::Distance(SpanStart, SpanEnd), Widget->GetDesiredSize().X, ViewportScale);
	const float ScreenMargin = Config->ScreenMargin;
	const FChopItPhoneIndicatorPlacement Placement = FChopItPhoneIndicatorProjection::Resolve(
		FVector2D(Width, Height), Projected, bProjected, LocalDirection, ScreenMargin, FVector2D::ZeroVector);
	const float EntranceAlpha = FMath::Clamp(RingElapsed / Config->EntranceDuration, 0.0f, 1.0f);
	const float Bounce = FMath::Sin(EntranceAlpha * PI) * Config->BounceOvershoot;
	const float BeatPhase = FMath::Clamp(BeatElapsed / 0.28f, 0.0f, 1.0f);
	const float BeatPulse = BeatPhase < 1.0f ? FMath::Sin(2.0f * PI * BeatPhase) * FMath::Sin(PI * BeatPhase) : 0.0f;
	const float BeatScaleKick = 0.07f * (1.0f - BeatPhase);
	const FString FullLabel = Config->Label.ToString();
	const int32 Characters = Config->TypeDuration > 0.0f
		? FMath::Clamp(FMath::CeilToInt(RingElapsed / Config->TypeDuration * FullLabel.Len()), 1, FullLabel.Len()) : FullLabel.Len();
	ViewModel->SetLabel(FText::FromString(FullLabel.Left(Characters)));
	ViewModel->SetScreenPosition(Placement.Position + BeatJitter * BaseScale * ViewportScale);
	ViewModel->SetScale(BaseScale * (0.55f + 0.45f * EntranceAlpha + Bounce + BeatScaleKick));
	ViewModel->SetOpacity(Opacity);
	ViewModel->SetArrowDegrees(Placement.ArrowDegrees);
	ViewModel->SetOffscreen(Placement.bOffscreen);
	ViewModel->SetRingPulse(BeatPulse);
	ViewModel->SetTiltDegrees(BeatTiltDegrees);
	ViewModel->SetVisible(true);
}
