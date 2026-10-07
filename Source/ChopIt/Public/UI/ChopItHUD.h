#pragma once

#include "GameFramework/HUD.h"
#include "ChopItHUD.generated.h"

class UChopItUpgradeDefinition;
class UChopItWeaponDefinition;
class UChopItPactDefinition;
class UUserWidget;
class UChopItItemDataAsset;
class UTexture2D;
class USoundBase;
class AChopItChestRevealScene;

/** Screen-space presentation for run state and level-up choices. It owns no gameplay rules. */
UCLASS()
class CHOPIT_API AChopItHUD final : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DrawHUD() override;

	/** Reveals the right-side objective card. Safe to call repeatedly. */
	void RevealMissionTracker();
	/** Presentation only; the chest owns the luck-weighted roll and awards the result. */
	void StartItemReveal(UChopItItemDataAsset* Result);
	void CompleteItemReveal(UChopItItemDataAsset* Result);
	void CancelItemReveal();
	bool IsItemRevealActive() const { return bItemRevealActive; }
	void DismissItemReveal();
	void ToggleDebugItemMenu();
	void CloseDebugItemMenu();
	bool IsDebugItemMenuOpen() const { return bDebugItemMenuOpen; }
	void MoveDebugItemSelection(int32 Delta);
	void GrantSelectedDebugItem();

private:
	void DrawPersistentHUD(float Scale);
	void DrawMissionTracker(float Scale);
	void DrawUpgradeOverlay(float Scale, const TArray<TObjectPtr<UChopItUpgradeDefinition>>& Offers);
	void DrawShopOverlay(float Scale, const TArray<TObjectPtr<UChopItWeaponDefinition>>& Offers);
	void DrawPactOverlay(float Scale, const TArray<TObjectPtr<UChopItPactDefinition>>& Offers, int32 Curse);
	void DrawDefeatOverlay(float Scale);
	void DrawVictoryOverlay(float Scale);
	void DrawItemReveal(float Scale);
	void PauseForItemUI();
	void ResumeFromItemUI();
	float GetItemRevealDuration() const;
	void DrawItemInventory(float Scale);
	void DrawBossHealthBar(float Scale);
	void DrawDebugItemMenu(float Scale);
	UTexture2D* ResolveItemIcon(const UChopItItemDataAsset* Item);
	void RefreshPSXWidget();
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

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> PSXHUDWidget;
	TMap<FName, float> PSXFillDesignWidths;
	UPROPERTY(Transient) TObjectPtr<UChopItItemDataAsset> ItemRevealResult;
	UPROPERTY(Transient) TObjectPtr<AChopItChestRevealScene> ChestRevealScene;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> ItemRevealIcon;
	UPROPERTY(Transient) TObjectPtr<USoundBase> ItemRevealTickSound;
	double ItemRevealStartedAt = 0.0;
	int32 LastRevealStage = INDEX_NONE;
	bool bItemRevealActive = false;
	bool bItemRevealGranted = false;
	bool bOwnsItemPause = false;
	UPROPERTY(Transient) TArray<TObjectPtr<UChopItItemDataAsset>> DebugItemCatalog;
	UPROPERTY(Transient) TMap<FName, TObjectPtr<UTexture2D>> ItemIconCache;
	int32 DebugItemSelection = 0;
	bool bDebugItemMenuOpen = false;
	FString DebugItemStatus;
};
