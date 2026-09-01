#pragma once

#include "GameFramework/HUD.h"
#include "ChopItHUD.generated.h"

class UChopItUpgradeDefinition;
class UChopItWeaponDefinition;
class UChopItPactDefinition;

/** Screen-space presentation for run state and level-up choices. It owns no gameplay rules. */
UCLASS()
class CHOPIT_API AChopItHUD final : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	/** Reveals the right-side objective card. Safe to call repeatedly. */
	void RevealMissionTracker();

private:
	void DrawPersistentHUD(float Scale);
	void DrawMissionTracker(float Scale);
	void DrawUpgradeOverlay(float Scale, const TArray<TObjectPtr<UChopItUpgradeDefinition>>& Offers);
	void DrawShopOverlay(float Scale, const TArray<TObjectPtr<UChopItWeaponDefinition>>& Offers);
	void DrawPactOverlay(float Scale, const TArray<TObjectPtr<UChopItPactDefinition>>& Offers, int32 Curse);
	void DrawDefeatOverlay(float Scale);
	void DrawVictoryOverlay(float Scale);
	void DrawPanel(float X, float Y, float Width, float Height, const FLinearColor& Fill, const FLinearColor& Border, float BorderSize = 3.0f);
	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Fill, const FLinearColor& Back);
	void DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, float TextScale = 1.0f, bool bLarge = false);
	void DrawCenteredLabel(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float TextScale = 1.0f, bool bLarge = false);

	bool bMissionTrackerRequested = false;
	bool bMissionTrackerDismissed = false;
	bool bMissionCompletionStarted = false;
	int32 LastMissionProgress = INDEX_NONE;
	int32 LastMissionTarget = INDEX_NONE;
	int32 LastMissionDelta = 0;
	double MissionRevealTime = 0.0;
	double MissionUpdateTime = -1000.0;
	double MissionCompletionTime = -1000.0;
};
