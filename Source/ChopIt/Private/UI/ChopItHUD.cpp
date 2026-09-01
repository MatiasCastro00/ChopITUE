#include "UI/ChopItHUD.h"

#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Cycle/ChopItRunStateComponent.h"
#include "Economy/ChopItEconomyComponent.h"
#include "Economy/ChopItQuotaComponent.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Engine/Engine.h"
#include "Framework/ChopItGameState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Harvest/ChopItWoodCargoComponent.h"
#include "Player/ChopItCharacter.h"
#include "Combat/ChopItHealthComponent.h"
#include "Progression/ChopItExperienceComponent.h"
#include "Progression/ChopItUpgradeDefinition.h"
#include "Progression/ChopItUpgradeOfferComponent.h"
#include "Shop/ChopItShopComponent.h"
#include "Weapons/ChopItWeaponDefinition.h"
#include "Pacts/ChopItPactComponent.h"
#include "Pacts/ChopItPactDefinition.h"

namespace ChopItHUD
{
	const FLinearColor Ink(0.055f, 0.045f, 0.035f, 0.94f);
	const FLinearColor Border(0.48f, 0.34f, 0.18f, 1.0f);
	const FLinearColor Cream(0.94f, 0.84f, 0.64f, 1.0f);
	const FLinearColor Orange(1.0f, 0.42f, 0.05f, 1.0f);
	const FLinearColor Green(0.22f, 0.86f, 0.22f, 1.0f);
	const FLinearColor Red(0.88f, 0.08f, 0.04f, 1.0f);

	FLinearColor WithAlpha(const FLinearColor& Color, const float Alpha)
	{
		FLinearColor Result = Color;
		Result.A *= FMath::Clamp(Alpha, 0.0f, 1.0f);
		return Result;
	}

	float EaseOutBack(const float Alpha)
	{
		const float T = FMath::Clamp(Alpha, 0.0f, 1.0f) - 1.0f;
		constexpr float C1 = 1.70158f;
		constexpr float C3 = C1 + 1.0f;
		return 1.0f + C3 * T * T * T + C1 * T * T;
	}

	void DrawSolidRect(UCanvas* Canvas, const float X, const float Y, const float Width, const float Height, const FLinearColor& Color)
	{
		FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(Width, Height), Color);
		Tile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Tile);
	}
}

void AChopItHUD::BeginPlay()
{
	Super::BeginPlay();
	// Development maps have no authored introduction, so their objective card
	// is available immediately. L_Startup reveals it on the QuestStart cue.
	if (!GetWorld() || !GetWorld()->GetMapName().Contains(TEXT("L_Startup")))
	{
		RevealMissionTracker();
	}
}

void AChopItHUD::RevealMissionTracker()
{
	if (bMissionTrackerRequested && !bMissionTrackerDismissed) return;
	bMissionTrackerRequested = true;
	bMissionTrackerDismissed = false;
	bMissionCompletionStarted = false;
	MissionRevealTime = FPlatformTime::Seconds();
}

void AChopItHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas || !PlayerOwner)
	{
		return;
	}
	// Keep the HUD comfortably readable at common 720p/768p window sizes. Layout
	// still scales up with the viewport, but it never shrinks into debug-text size.
	const float Scale = FMath::Clamp(FMath::Min(Canvas->SizeX / 1920.0f, Canvas->SizeY / 1080.0f), 0.80f, 1.5f);
	DrawPersistentHUD(Scale);
	DrawMissionTracker(Scale);

	const APlayerState* State = PlayerOwner->PlayerState;
	const AChopItGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChopItGameState>() : nullptr;
	if (GameState && GameState->GetCycleStateMachine()
		&& GameState->GetCycleStateMachine()->GetCurrentPhase() == EChopItCyclePhase::Death)
	{
		DrawDefeatOverlay(Scale);
		return;
	}
	if (GameState && GameState->GetCycleStateMachine()
		&& GameState->GetCycleStateMachine()->GetCurrentPhase() == EChopItCyclePhase::Victory)
	{
		DrawVictoryOverlay(Scale);
		return;
	}
	const UChopItUpgradeOfferComponent* Offers = State
		? State->FindComponentByClass<UChopItUpgradeOfferComponent>() : nullptr;
	if (Offers && Offers->HasActiveOffer())
	{
		DrawUpgradeOverlay(Scale, Offers->GetActiveOffers());
		return;
	}
	const UChopItShopComponent* Shop = State ? State->FindComponentByClass<UChopItShopComponent>() : nullptr;
	const UChopItPactComponent* Pacts = State ? State->FindComponentByClass<UChopItPactComponent>() : nullptr;
	if (Pacts && Pacts->HasActiveOffer()) { DrawPactOverlay(Scale, Pacts->GetActiveOffers(), Pacts->GetCurse()); return; }
	if (Shop && Shop->HasActiveShop())
	{
		DrawShopOverlay(Scale, Shop->GetActiveOffers());
	}
}

void AChopItHUD::DrawMissionTracker(const float Scale)
{
	if (!Canvas || !PlayerOwner || !bMissionTrackerRequested) return;
	const AChopItGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChopItGameState>() : nullptr;
	const UChopItQuotaComponent* Quota = GameState ? GameState->GetQuotaComponent() : nullptr;
	if (!Quota || Quota->GetTarget() <= 0) return;

	const double Now = FPlatformTime::Seconds();
	const int32 Progress = Quota->GetProgress();
	const int32 Target = Quota->GetTarget();
	if (LastMissionTarget != Target)
	{
		LastMissionTarget = Target;
		LastMissionProgress = Progress;
		LastMissionDelta = 0;
		bMissionTrackerDismissed = false;
		bMissionCompletionStarted = false;
		MissionRevealTime = Now;
	}
	else if (LastMissionProgress != Progress)
	{
		LastMissionDelta = Progress - FMath::Max(0, LastMissionProgress);
		LastMissionProgress = Progress;
		MissionUpdateTime = Now;
	}

	if (Quota->IsComplete() && !bMissionCompletionStarted)
	{
		bMissionCompletionStarted = true;
		MissionCompletionTime = Now;
		MissionUpdateTime = Now;
	}
	if (bMissionTrackerDismissed) return;

	constexpr float EnterDuration = 0.58f;
	constexpr float CelebrationDuration = 1.05f;
	constexpr float ExitDuration = 0.48f;
	const float EnterAlpha = FMath::Clamp(static_cast<float>((Now - MissionRevealTime) / EnterDuration), 0.0f, 1.0f);
	const float EnterEase = ChopItHUD::EaseOutBack(EnterAlpha);
	float Opacity = FMath::Clamp(EnterAlpha * 1.7f, 0.0f, 1.0f);
	float ExitAlpha = 0.0f;
	if (bMissionCompletionStarted)
	{
		const float CompletionElapsed = static_cast<float>(Now - MissionCompletionTime);
		ExitAlpha = FMath::Clamp((CompletionElapsed - CelebrationDuration) / ExitDuration, 0.0f, 1.0f);
		Opacity *= 1.0f - FMath::SmoothStep(0.0f, 1.0f, ExitAlpha);
		if (CompletionElapsed >= CelebrationDuration + ExitDuration)
		{
			bMissionTrackerDismissed = true;
			return;
		}
	}

	const float UpdateAlpha = FMath::Clamp(static_cast<float>((Now - MissionUpdateTime) / 0.46), 0.0f, 1.0f);
	const float UpdatePunch = FMath::Sin(UpdateAlpha * PI) * (1.0f - ExitAlpha);
	const float CompletionPulse = bMissionCompletionStarted
		? FMath::Sin(FMath::Clamp(static_cast<float>((Now - MissionCompletionTime) / 0.72), 0.0f, 1.0f) * PI) : 0.0f;
	const float CardScale = 1.0f + UpdatePunch * 0.075f + CompletionPulse * 0.055f;
	const float BaseWidth = 350.0f * Scale;
	const float BaseHeight = 158.0f * Scale;
	const float Width = BaseWidth * CardScale;
	const float Height = BaseHeight * CardScale;
	const float SlideIn = (1.0f - EnterEase) * (BaseWidth + 52.0f * Scale);
	const float SlideOut = FMath::SmoothStep(0.0f, 1.0f, ExitAlpha) * (BaseWidth + 70.0f * Scale);
	const float X = Canvas->SizeX - 26.0f * Scale - Width + SlideIn + SlideOut - UpdatePunch * 12.0f * Scale;
	const float Y = 82.0f * Scale - (Height - BaseHeight) * 0.5f;
	const float FontScale = FMath::Clamp(Scale * 1.08f, 0.90f, 1.48f);
	const bool bComplete = Quota->IsComplete();
	const FLinearColor Accent = bComplete ? ChopItHUD::Green : ChopItHUD::Orange;
	const FLinearColor FlashBorder = FMath::Lerp(Accent, ChopItHUD::Cream, UpdatePunch * 0.75f);

	DrawPanel(X + 8.0f * Scale, Y + 9.0f * Scale, Width, Height,
		ChopItHUD::WithAlpha(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), Opacity),
		ChopItHUD::WithAlpha(FLinearColor(0.0f, 0.0f, 0.0f, 0.15f), Opacity), 2.0f * Scale);
	DrawPanel(X, Y, Width, Height,
		ChopItHUD::WithAlpha(FLinearColor(0.075f, 0.052f, 0.032f, 0.97f), Opacity),
		ChopItHUD::WithAlpha(FlashBorder, Opacity), (4.0f + UpdatePunch * 3.0f) * Scale);
	ChopItHUD::DrawSolidRect(Canvas, X + 8.0f * Scale, Y + 8.0f * Scale,
		7.0f * Scale, Height - 16.0f * Scale, ChopItHUD::WithAlpha(Accent, Opacity));

	// Three compact blocks read as a small stack of logs without requiring a texture asset.
	for (int32 LogIndex = 0; LogIndex < 3; ++LogIndex)
	{
		ChopItHUD::DrawSolidRect(Canvas,
			X + (24.0f + LogIndex * 7.0f) * Scale,
			Y + (25.0f - LogIndex * 5.0f) * Scale,
			25.0f * Scale, 8.0f * Scale,
			ChopItHUD::WithAlpha(FLinearColor(0.56f, 0.25f, 0.065f, 1.0f), Opacity));
	}

	DrawLabel(bComplete ? TEXT("MISSION COMPLETE") : TEXT("ACTIVE MISSION"),
		X + 68.0f * Scale, Y + 16.0f * Scale,
		ChopItHUD::WithAlpha(Accent, Opacity), 0.92f * FontScale, true);
	DrawLabel(TEXT("FEED THE OVEN"), X + 24.0f * Scale, Y + 52.0f * Scale,
		ChopItHUD::WithAlpha(ChopItHUD::Cream, Opacity), 0.88f * FontScale, true);
	const int32 Remaining = FMath::Max(0, Target - Progress);
	DrawLabel(bComplete ? TEXT("QUOTA FULFILLED") : FString::Printf(TEXT("WOOD REMAINING:  %d"), Remaining),
		X + 24.0f * Scale, Y + 84.0f * Scale,
		ChopItHUD::WithAlpha(bComplete ? ChopItHUD::Green : FLinearColor::White, Opacity),
		(1.0f + UpdatePunch * 0.12f) * FontScale);
	DrawBar(X + 24.0f * Scale, Y + 121.0f * Scale, Width - 48.0f * Scale, 13.0f * Scale,
		static_cast<float>(Progress) / FMath::Max(1, Target),
		ChopItHUD::WithAlpha(Accent, Opacity),
		ChopItHUD::WithAlpha(FLinearColor(0.12f, 0.09f, 0.055f, 1.0f), Opacity));
	DrawLabel(FString::Printf(TEXT("%d / %d"), Progress, Target),
		X + Width - 88.0f * Scale, Y + 136.0f * Scale,
		ChopItHUD::WithAlpha(ChopItHUD::Cream, Opacity), 0.66f * FontScale);

	if (UpdatePunch > 0.01f && LastMissionDelta > 0)
	{
		DrawLabel(FString::Printf(TEXT("+%d"), LastMissionDelta),
			X - 18.0f * Scale, Y + 62.0f * Scale - UpdatePunch * 22.0f * Scale,
			ChopItHUD::WithAlpha(Accent, Opacity * UpdatePunch), 1.1f * FontScale, true);
	}
	const float SparkStrength = FMath::Max(UpdatePunch, CompletionPulse);
	for (int32 SparkIndex = 0; SparkIndex < 7 && SparkStrength > 0.01f; ++SparkIndex)
	{
		const float Angle = (2.0f * PI * SparkIndex / 7.0f) + 0.35f;
		const float Radius = (18.0f + 34.0f * (1.0f - SparkStrength)) * Scale;
		const float SparkSize = (3.0f + 4.0f * SparkStrength) * Scale;
		ChopItHUD::DrawSolidRect(Canvas,
			X + 18.0f * Scale + FMath::Cos(Angle) * Radius,
			Y + 20.0f * Scale + FMath::Sin(Angle) * Radius,
			SparkSize, SparkSize,
			ChopItHUD::WithAlpha(Accent, Opacity * SparkStrength));
	}
}

void AChopItHUD::DrawDefeatOverlay(const float Scale)
{
	ChopItHUD::DrawSolidRect(Canvas, 0.0f, 0.0f, Canvas->SizeX, Canvas->SizeY, FLinearColor(0.08f, 0.0f, 0.0f, 0.86f));
	const float CenterX = Canvas->SizeX * 0.5f;
	const float FontScale = FMath::Clamp(Scale * 1.2f, 0.95f, 1.65f);
	DrawCenteredLabel(TEXT("DEFEAT"), CenterX, Canvas->SizeY * 0.36f, ChopItHUD::Red, 2.0f * FontScale, true);
	DrawCenteredLabel(TEXT("Your expedition is over. Restart the game to try again."),
		CenterX, Canvas->SizeY * 0.47f, ChopItHUD::Cream, FontScale);
}

void AChopItHUD::DrawVictoryOverlay(const float Scale)
{
	ChopItHUD::DrawSolidRect(Canvas, 0.0f, 0.0f, Canvas->SizeX, Canvas->SizeY, FLinearColor(0.02f, 0.10f, 0.03f, 0.88f));
	const float CenterX = Canvas->SizeX * 0.5f;
	const float FontScale = FMath::Clamp(Scale * 1.2f, 0.95f, 1.65f);
	DrawCenteredLabel(TEXT("THE FOREST HAS BEEN DEFEATED"), CenterX, Canvas->SizeY * 0.25f, ChopItHUD::Green, 1.45f * FontScale, true);
	DrawCenteredLabel(TEXT("[1] Retire and claim the victory reward"), CenterX, Canvas->SizeY * 0.43f, ChopItHUD::Cream, FontScale, true);
	DrawCenteredLabel(TEXT("[2] Enter the endless night"), CenterX, Canvas->SizeY * 0.52f, ChopItHUD::Orange, FontScale, true);
	DrawCenteredLabel(TEXT("There will be no other dawn: the waves grow until you die."), CenterX, Canvas->SizeY * 0.62f, FLinearColor::White, 0.85f * FontScale);
}

void AChopItHUD::DrawPactOverlay(float Scale, const TArray<TObjectPtr<UChopItPactDefinition>>& Offers, int32 Curse)
{
	ChopItHUD::DrawSolidRect(Canvas,0,0,Canvas->SizeX,Canvas->SizeY,FLinearColor(0,0,0,0.78f));
	const float CX=Canvas->SizeX*0.5f; const float F=FMath::Clamp(Scale*1.15f,0.95f,1.65f);
	DrawCenteredLabel(TEXT("THE REAPER OFFERS A PACT"),CX,120*Scale,ChopItHUD::Red,1.4f*F,true);
	DrawCenteredLabel(FString::Printf(TEXT("CURSE %d  |  Choose with 1, 2 or 3"),Curse),CX,175*Scale,ChopItHUD::Cream,F);
	for(int32 I=0;I<Offers.Num()&&I<3;++I) if(const UChopItPactDefinition* P=Offers[I]) { float X=CX+(I-1)*330*Scale; DrawPanel(X-140*Scale,240*Scale,280*Scale,280*Scale,ChopItHUD::Ink,ChopItHUD::Red,5*Scale); DrawCenteredLabel(P->DisplayName.ToString(),X,285*Scale,ChopItHUD::Cream,F,true); DrawCenteredLabel(P->Description.ToString(),X,365*Scale,FLinearColor::White,F); DrawCenteredLabel(FString::Printf(TEXT("[%d] +%d curse"),I+1,P->CurseIncrease),X,450*Scale,ChopItHUD::Orange,F); }
}

void AChopItHUD::DrawPersistentHUD(const float Scale)
{
	const float FontScale = FMath::Clamp(Scale * 1.15f, 0.95f, 1.65f);
	const AChopItGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChopItGameState>() : nullptr;
	const APlayerState* State = PlayerOwner->PlayerState;
	const AChopItCharacter* Character = Cast<AChopItCharacter>(PlayerOwner->GetPawn());
	if (!GameState || !State || !Character)
	{
		return;
	}

	const UChopItRunStateComponent* Run = GameState->GetRunStateComponent();
	const UChopItCycleStateMachineComponent* Cycle = GameState->GetCycleStateMachine();
	const UChopItQuotaComponent* Quota = GameState->GetQuotaComponent();
	const UChopItExperienceComponent* Experience = State->FindComponentByClass<UChopItExperienceComponent>();
	const UChopItEconomyComponent* Economy = State->FindComponentByClass<UChopItEconomyComponent>();
	const UChopItWoodCargoComponent* Cargo = Character->GetWoodCargoComponent();

	const float X = 24.0f * Scale;
	const float Y = 24.0f * Scale;
	const float Width = 430.0f * Scale;
	const float Height = 220.0f * Scale;
	DrawPanel(X, Y, Width, Height, ChopItHUD::Ink, ChopItHUD::Border, 4.0f * Scale);

	const EChopItCyclePhase Phase = Cycle ? Cycle->GetCurrentPhase() : EChopItCyclePhase::Bootstrap;
	const TCHAR* PhaseName = TEXT("STARTING");
	switch (Phase)
	{
	case EChopItCyclePhase::Day: PhaseName = TEXT("DAY"); break;
	case EChopItCyclePhase::Dusk: PhaseName = TEXT("DUSK"); break;
	case EChopItCyclePhase::Night: PhaseName = TEXT("NIGHT"); break;
	case EChopItCyclePhase::Elite: PhaseName = TEXT("ELITE"); break;
	case EChopItCyclePhase::Resolution: PhaseName = TEXT("CYCLE COMPLETE"); break;
	case EChopItCyclePhase::Death: PhaseName = TEXT("DEFEAT"); break;
	case EChopItCyclePhase::Victory: PhaseName = TEXT("VICTORY"); break;
	default: break;
	}
	const int32 Day = Run ? Run->GetDayNumber() : 1;
	const float Remaining = Cycle ? Cycle->GetPhaseRemaining() : -1.0f;
	DrawLabel(FString::Printf(TEXT("DAY %d  |  %s"), Day, PhaseName), X + 18.0f * Scale, Y + 12.0f * Scale, ChopItHUD::Cream, 1.15f * FontScale, true);
	DrawLabel(Remaining >= 0.0f ? FString::Printf(TEXT("TIME  %02.0fs"), Remaining) : TEXT("TIME  --"), X + 18.0f * Scale, Y + 50.0f * Scale, FLinearColor::White, FontScale);
	const UChopItHealthComponent* Health = Character->GetHealthComponent();
	DrawLabel(FString::Printf(TEXT("HEALTH  %.0f / %.0f"), Health ? Health->GetCurrentHealth() : 0.0f, Health ? Health->GetMaxHealth() : 0.0f), X + 18.0f * Scale, Y + 78.0f * Scale, ChopItHUD::Red, FontScale);

	const int32 QuotaProgress = Quota ? Quota->GetProgress() : 0;
	const int32 QuotaTarget = Quota ? Quota->GetTarget() : 1;
	DrawLabel(FString::Printf(TEXT("QUOTA  %d / %d"), QuotaProgress, QuotaTarget), X + 18.0f * Scale, Y + 110.0f * Scale, ChopItHUD::Cream, FontScale);
	DrawBar(X + 160.0f * Scale, Y + 116.0f * Scale, 245.0f * Scale, 14.0f * Scale,
		static_cast<float>(QuotaProgress) / FMath::Max(1, QuotaTarget), ChopItHUD::Orange, FLinearColor(0.12f, 0.1f, 0.08f));

	DrawLabel(FString::Printf(TEXT("WOOD  %d / %d"), Cargo ? Cargo->GetCurrentWood() : 0, Cargo ? Cargo->GetCapacity() : 0),
		X + 18.0f * Scale, Y + 148.0f * Scale, FLinearColor(0.95f, 0.72f, 0.18f), FontScale);
	DrawLabel(FString::Printf(TEXT("MONEY  $%lld"), Economy ? Economy->GetBalance() : 0), X + 230.0f * Scale, Y + 148.0f * Scale, ChopItHUD::Cream, FontScale);

	if (Phase == EChopItCyclePhase::Dusk || Phase == EChopItCyclePhase::Night)
	{
		const FString Guidance = Phase == EChopItCyclePhase::Night && Cycle && Cycle->IsInfiniteMode()
			? TEXT(">> ENDLESS NIGHT")
			: Phase == EChopItCyclePhase::Night
			? FString::Printf(TEXT(">> ELITE ARRIVES IN %.0fs"), FMath::Max(0.0f, Remaining))
			: TEXT(">> RETURN TO THE CABIN / LEVER");
		DrawLabel(Guidance, X + 18.0f * Scale, Y + 184.0f * Scale, ChopItHUD::Orange, 0.9f * FontScale);
	}

	const int32 Level = Experience ? Experience->GetLevel() : 1;
	const int32 XP = Experience ? Experience->GetCurrentExperience() : 0;
	const int32 RequiredXP = Experience ? Experience->GetRequiredExperience() : 1;
	const float BarX = 260.0f * Scale;
	const float BarY = Canvas->SizeY - 58.0f * Scale;
	const float BarWidth = Canvas->SizeX - 520.0f * Scale;
	DrawBar(BarX, BarY, BarWidth, 24.0f * Scale, static_cast<float>(XP) / FMath::Max(1, RequiredXP), ChopItHUD::Green, FLinearColor(0.06f, 0.08f, 0.05f));
	DrawPanel(Canvas->SizeX * 0.5f - 66.0f * Scale, BarY - 8.0f * Scale, 132.0f * Scale, 40.0f * Scale, ChopItHUD::Ink, ChopItHUD::Border, 3.0f * Scale);
	DrawCenteredLabel(FString::Printf(TEXT("LEVEL %d"), Level), Canvas->SizeX * 0.5f, BarY, ChopItHUD::Cream, FontScale, true);
	DrawLabel(FString::Printf(TEXT("XP %d / %d"), XP, RequiredXP), BarX, BarY - 26.0f * Scale, ChopItHUD::Cream, 0.8f * FontScale);
}

void AChopItHUD::DrawUpgradeOverlay(
	const float Scale,
	const TArray<TObjectPtr<UChopItUpgradeDefinition>>& Offers)
{
	const float FontScale = FMath::Clamp(Scale * 1.15f, 0.95f, 1.65f);
	ChopItHUD::DrawSolidRect(Canvas, 0.0f, 0.0f, Canvas->SizeX, Canvas->SizeY, FLinearColor(0.0f, 0.0f, 0.0f, 0.80f));
	const float CenterX = Canvas->SizeX * 0.5f;
	DrawCenteredLabel(TEXT("LEVEL UP"), CenterX, 135.0f * Scale, ChopItHUD::Cream, 1.55f * FontScale, true);
	DrawCenteredLabel(TEXT("Choose an upgrade with 1, 2 or 3"), CenterX, 190.0f * Scale, FLinearColor::White, FontScale);

	const float CardWidth = 300.0f * Scale;
	const float CardHeight = 360.0f * Scale;
	const float Gap = 28.0f * Scale;
	const float TotalWidth = CardWidth * 3.0f + Gap * 2.0f;
	const float StartX = CenterX - TotalWidth * 0.5f;
	const float CardY = 250.0f * Scale;
	for (int32 Index = 0; Index < Offers.Num() && Index < 3; ++Index)
	{
		const UChopItUpgradeDefinition* Upgrade = Offers[Index];
		if (!Upgrade)
		{
			continue;
		}
		FLinearColor RarityColor(0.25f, 0.72f, 0.18f, 1.0f);
		if (Upgrade->Rarity == EChopItUpgradeRarity::Uncommon) { RarityColor = FLinearColor(0.12f, 0.58f, 0.86f, 1.0f); }
		if (Upgrade->Rarity == EChopItUpgradeRarity::Rare) { RarityColor = FLinearColor(1.0f, 0.38f, 0.02f, 1.0f); }
		const float CardX = StartX + Index * (CardWidth + Gap);
		DrawPanel(CardX, CardY, CardWidth, CardHeight, ChopItHUD::Ink, RarityColor, 6.0f * Scale);
		DrawPanel(CardX + 22.0f * Scale, CardY + 22.0f * Scale, CardWidth - 44.0f * Scale, 52.0f * Scale,
			FLinearColor(RarityColor.R * 0.25f, RarityColor.G * 0.25f, RarityColor.B * 0.25f, 1.0f), RarityColor, 2.0f * Scale);
		DrawCenteredLabel(FString::Printf(TEXT("OPTION %d"), Index + 1), CardX + CardWidth * 0.5f, CardY + 33.0f * Scale, ChopItHUD::Cream, FontScale, true);
		DrawCenteredLabel(Upgrade->DisplayName.ToString().ToUpper(), CardX + CardWidth * 0.5f, CardY + 125.0f * Scale, ChopItHUD::Cream, 1.05f * FontScale, true);
		DrawCenteredLabel(Upgrade->Description.ToString(), CardX + CardWidth * 0.5f, CardY + 195.0f * Scale, FLinearColor(0.58f, 1.0f, 0.36f), FontScale);
		DrawPanel(CardX + 105.0f * Scale, CardY + 290.0f * Scale, 90.0f * Scale, 48.0f * Scale, RarityColor, ChopItHUD::Cream, 2.0f * Scale);
		DrawCenteredLabel(FString::FromInt(Index + 1), CardX + CardWidth * 0.5f, CardY + 296.0f * Scale, FLinearColor::White, 1.2f * FontScale, true);
	}
}

void AChopItHUD::DrawShopOverlay(
	const float Scale,
	const TArray<TObjectPtr<UChopItWeaponDefinition>>& Offers)
{
	const float FontScale = FMath::Clamp(Scale * 1.15f, 0.95f, 1.65f);
	ChopItHUD::DrawSolidRect(Canvas, 0.0f, 0.0f, Canvas->SizeX, Canvas->SizeY, FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
	const float CenterX = Canvas->SizeX * 0.5f;
	DrawCenteredLabel(TEXT("TOOL SHOP"), CenterX, 135.0f * Scale, ChopItHUD::Cream, 1.35f * FontScale, true);
	DrawCenteredLabel(TEXT("Choose a weapon with 1, 2 or 3  |  ESC to close"), CenterX, 190.0f * Scale, FLinearColor::White, FontScale);

	const float CardWidth = 300.0f * Scale;
	const float CardHeight = 330.0f * Scale;
	const float Gap = 28.0f * Scale;
	const float TotalWidth = CardWidth * Offers.Num() + Gap * FMath::Max(0, Offers.Num() - 1);
	const float StartX = CenterX - TotalWidth * 0.5f;
	const float CardY = 250.0f * Scale;
	for (int32 Index = 0; Index < Offers.Num() && Index < 3; ++Index)
	{
		const UChopItWeaponDefinition* Weapon = Offers[Index];
		if (!Weapon) { continue; }
		const FLinearColor Accent = Weapon->AttackPattern == EChopItWeaponAttackPattern::RadialMelee
			? FLinearColor(0.10f, 0.65f, 0.90f, 1.0f) : ChopItHUD::Orange;
		const float CardX = StartX + Index * (CardWidth + Gap);
		DrawPanel(CardX, CardY, CardWidth, CardHeight, ChopItHUD::Ink, Accent, 6.0f * Scale);
		DrawPanel(CardX + 22.0f * Scale, CardY + 22.0f * Scale, CardWidth - 44.0f * Scale, 52.0f * Scale,
			FLinearColor(Accent.R * 0.25f, Accent.G * 0.25f, Accent.B * 0.25f, 1.0f), Accent, 2.0f * Scale);
		DrawCenteredLabel(FString::Printf(TEXT("OPTION %d"), Index + 1), CardX + CardWidth * 0.5f, CardY + 33.0f * Scale, ChopItHUD::Cream, FontScale, true);
		DrawCenteredLabel(Weapon->DisplayName.ToString().ToUpper(), CardX + CardWidth * 0.5f, CardY + 120.0f * Scale, ChopItHUD::Cream, 1.05f * FontScale, true);
		DrawCenteredLabel(Weapon->Description.ToString(), CardX + CardWidth * 0.5f, CardY + 180.0f * Scale, FLinearColor(0.58f, 1.0f, 0.36f), FontScale);
		DrawCenteredLabel(FString::Printf(TEXT("$%lld"), Weapon->ShopPrice), CardX + CardWidth * 0.5f, CardY + 235.0f * Scale, ChopItHUD::Cream, 1.2f * FontScale, true);
		DrawPanel(CardX + 105.0f * Scale, CardY + 265.0f * Scale, 90.0f * Scale, 44.0f * Scale, Accent, ChopItHUD::Cream, 2.0f * Scale);
		DrawCenteredLabel(FString::FromInt(Index + 1), CardX + CardWidth * 0.5f, CardY + 270.0f * Scale, FLinearColor::White, 1.05f * FontScale, true);
	}
}

void AChopItHUD::DrawPanel(
	const float X, const float Y, const float Width, const float Height,
	const FLinearColor& Fill, const FLinearColor& Border, const float BorderSize)
{
	ChopItHUD::DrawSolidRect(Canvas, X, Y, Width, Height, Border);
	ChopItHUD::DrawSolidRect(Canvas, X + BorderSize, Y + BorderSize,
		Width - BorderSize * 2.0f, Height - BorderSize * 2.0f, Fill);
}

void AChopItHUD::DrawBar(
	const float X, const float Y, const float Width, const float Height, const float Fraction,
	const FLinearColor& Fill, const FLinearColor& Back)
{
	FLinearColor BarBorder = ChopItHUD::Border;
	BarBorder.A *= FMath::Max(Back.A, Fill.A);
	DrawPanel(X, Y, Width, Height, Back, BarBorder, 2.0f);
	ChopItHUD::DrawSolidRect(Canvas, X + 3.0f, Y + 3.0f,
		(Width - 6.0f) * FMath::Clamp(Fraction, 0.0f, 1.0f), Height - 6.0f, Fill);
}

void AChopItHUD::DrawLabel(
	const FString& Text, const float X, const float Y, const FLinearColor& Color,
	const float TextScale, const bool bLarge)
{
	UFont* Font = bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont();
	Canvas->SetDrawColor(Color.ToFColor(true));
	Canvas->DrawText(Font, Text, X, Y, TextScale, TextScale);
}

void AChopItHUD::DrawCenteredLabel(
	const FString& Text, const float CenterX, const float Y, const FLinearColor& Color,
	const float TextScale, const bool bLarge)
{
	UFont* Font = bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont();
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	Canvas->StrLen(Font, Text, TextWidth, TextHeight);
	DrawLabel(Text, CenterX - TextWidth * TextScale * 0.5f, Y, Color, TextScale, bLarge);
}
