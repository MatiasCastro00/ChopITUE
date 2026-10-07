#include "UI/ChopItHUD.h"
#include "UI/ChopItJuiceWidget.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "Cycle/ChopItRunStateComponent.h"
#include "Economy/ChopItEconomyComponent.h"
#include "Economy/ChopItQuotaComponent.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Engine/Engine.h"
#include "Engine/AssetManager.h"
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
#include "Items/ChopItItemDataAsset.h"
#include "Items/ChopItItemComponent.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Rewards/ChopItChestRevealScene.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Spawning/ChopItEliteEncounterComponent.h"
#include "Enemies/ChopItEnemyCharacter.h"
#include "Enemies/ChopItEnemyDefinition.h"

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
	static const TCHAR* WidgetPath = TEXT("/Game/ChopIt/UI/WBP_PSX_HUD.WBP_PSX_HUD_C");
	if (const TSubclassOf<UUserWidget> WidgetClass = LoadClass<UUserWidget>(nullptr, WidgetPath))
	{
		PSXHUDWidget = CreateWidget<UUserWidget>(PlayerOwner, WidgetClass);
		if (PSXHUDWidget) PSXHUDWidget->AddToViewport(0);
	}
	// Development maps have no authored introduction, so their objective card
	// is available immediately. L_Startup reveals it on the QuestStart cue.
	if (!GetWorld() || !GetWorld()->GetMapName().Contains(TEXT("L_Startup")))
	{
		RevealMissionTracker();
	}
}

void AChopItHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(ChestRevealScene)) ChestRevealScene->Destroy();
	ResumeFromItemUI();
	Super::EndPlay(EndPlayReason);
}

void AChopItHUD::RevealMissionTracker()
{
	if (bMissionTrackerRequested && !bMissionTrackerDismissed) return;
	bMissionTrackerRequested = true;
	bMissionTrackerDismissed = false;
	bMissionCompletionStarted = false;
	MissionRevealTime = FPlatformTime::Seconds();
	if (auto* Juice = Cast<UChopItJuiceWidget>(PSXHUDWidget)) Juice->RevealMission();
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
	RefreshPSXWidget();
	if (bItemRevealActive)
	{
		DrawItemReveal(Scale);
		return;
	}
	if (!PSXHUDWidget)
	{
		DrawPersistentHUD(Scale);
		DrawMissionTracker(Scale);
	}
	DrawItemInventory(Scale);
	DrawBossHealthBar(Scale);
	if (bDebugItemMenuOpen)
	{
		DrawDebugItemMenu(Scale);
		return;
	}

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

void AChopItHUD::DrawBossHealthBar(float Scale)
{
	const AChopItGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChopItGameState>() : nullptr;
	const UChopItEliteEncounterComponent* Encounter = GameState ? GameState->GetEliteEncounterComponent() : nullptr;
	const AChopItEnemyCharacter* Boss = Encounter ? Encounter->GetActiveElite() : nullptr;
	const UChopItHealthComponent* Health = IsValid(Boss) ? Boss->GetHealthComponent() : nullptr;
	if (!Health || !Health->IsAlive()) return;
	const float Width = FMath::Min(Canvas->SizeX * .62f, 850.f * Scale);
	const float X = (Canvas->SizeX - Width) * .5f;
	const float Y = Canvas->SizeY - 155.f * Scale;
	const FLinearColor Accent(.92f,.21f,.10f);
	DrawPanel(X,Y,Width,77.f*Scale,ChopItHUD::Ink,Accent,3.f*Scale);
	const UChopItEnemyDefinition* Definition = Boss->GetDefinition();
	DrawCenteredLabel(Definition ? Definition->DisplayName.ToString() : TEXT("BOSS"),
		Canvas->SizeX*.5f,Y+5.f*Scale,ChopItHUD::Cream,.88f*Scale,true);
	DrawBar(X+19.f*Scale,Y+38.f*Scale,Width-38.f*Scale,20.f*Scale,
		Health->GetCurrentHealth()/FMath::Max(1.f,Health->GetMaxHealth()),Accent,FLinearColor(.12f,.04f,.04f));
	DrawCenteredLabel(FString::Printf(TEXT("%.0f / %.0f"),Health->GetCurrentHealth(),Health->GetMaxHealth()),
		Canvas->SizeX*.5f,Y+38.f*Scale,FLinearColor::White,.7f*Scale);
}

UTexture2D* AChopItHUD::ResolveItemIcon(const UChopItItemDataAsset* Item)
{
	if (!Item) return nullptr;
	if (TObjectPtr<UTexture2D>* Cached = ItemIconCache.Find(Item->ItemId)) return Cached->Get();
	// The Data Asset is authoritative so designers can change its photo in Details.
	UTexture2D* Icon = Item->Icon.LoadSynchronous();
	if (!Icon)
	{
		const FString Stem = Item->ItemId == TEXT("GenerousLog") ? TEXT("HollowLog") : Item->ItemId.ToString();
		const FString Name = FString(TEXT("T_Stick_")) + Stem;
		const FString Path = FString::Printf(TEXT("/Game/ChopIt/Art/Sticks/%s.%s"), *Name, *Name);
		Icon = LoadObject<UTexture2D>(nullptr, *Path);
	}
	ItemIconCache.Add(Item->ItemId, Icon);
	return Icon;
}

void AChopItHUD::ToggleDebugItemMenu()
{
	if (bDebugItemMenuOpen) { CloseDebugItemMenu(); return; }
	if (bItemRevealActive) return;
	DebugItemCatalog.Reset();
	UAssetManager& Manager = UAssetManager::Get();
	TArray<FPrimaryAssetId> Ids;
	Manager.GetPrimaryAssetIdList(TEXT("ChopItItem"), Ids);
	for (const FPrimaryAssetId& Id : Ids)
	{
		if (UChopItItemDataAsset* Item = Cast<UChopItItemDataAsset>(Manager.GetPrimaryAssetPath(Id).TryLoad()))
			DebugItemCatalog.Add(Item);
	}
	DebugItemCatalog.Sort([](const UChopItItemDataAsset& A, const UChopItItemDataAsset& B)
	{
		return A.DisplayName.ToString() < B.DisplayName.ToString();
	});
	DebugItemSelection = 0;
	DebugItemStatus = DebugItemCatalog.IsEmpty() ? TEXT("NO SE ENCONTRARON DATA ASSETS") : FString();
	bDebugItemMenuOpen = true;
	if (PSXHUDWidget) PSXHUDWidget->SetVisibility(ESlateVisibility::Hidden);
	PauseForItemUI();
}

void AChopItHUD::CloseDebugItemMenu()
{
	bDebugItemMenuOpen = false;
	if (PSXHUDWidget) PSXHUDWidget->SetVisibility(ESlateVisibility::Visible);
	ResumeFromItemUI();
}

void AChopItHUD::PauseForItemUI()
{
	if (PlayerOwner && !PlayerOwner->IsPaused()) bOwnsItemPause = PlayerOwner->SetPause(true);
}

void AChopItHUD::ResumeFromItemUI()
{
	if (bOwnsItemPause && IsValid(PlayerOwner)) PlayerOwner->SetPause(false);
	bOwnsItemPause = false;
}

void AChopItHUD::MoveDebugItemSelection(const int32 Delta)
{
	if (bDebugItemMenuOpen && !DebugItemCatalog.IsEmpty())
		DebugItemSelection = FMath::Clamp(DebugItemSelection + Delta, 0, DebugItemCatalog.Num() - 1);
}

void AChopItHUD::GrantSelectedDebugItem()
{
	if (!bDebugItemMenuOpen || !DebugItemCatalog.IsValidIndex(DebugItemSelection)) return;
	UChopItItemComponent* Inventory = PlayerOwner && PlayerOwner->GetPawn()
		? PlayerOwner->GetPawn()->FindComponentByClass<UChopItItemComponent>() : nullptr;
	UChopItItemDataAsset* Item = DebugItemCatalog[DebugItemSelection];
	if (!Inventory || !Inventory->AddItem(Item))
	{
		DebugItemStatus = TEXT("NO SE PUDO AGREGAR EL ITEM");
		return;
	}
	DebugItemStatus = FString::Printf(TEXT("+1 %s  |  STACKS: %d"), *Item->DisplayName.ToString(),
		Inventory->GetStackCount(Item->ItemId));
}

void AChopItHUD::DrawItemInventory(const float Scale)
{
	const UChopItItemComponent* Inventory = PlayerOwner && PlayerOwner->GetPawn()
		? PlayerOwner->GetPawn()->FindComponentByClass<UChopItItemComponent>() : nullptr;
	if (!Inventory) return;
	TArray<FChopItOwnedItem> Owned = Inventory->GetItems();
	Owned.RemoveAll([](const FChopItOwnedItem& Entry) { return !Entry.Item || Entry.Stacks <= 0; });
	Owned.Sort([](const FChopItOwnedItem& A, const FChopItOwnedItem& B)
	{
		return A.Item->DisplayName.ToString() < B.Item->DisplayName.ToString();
	});
	const float Width = FMath::Min(440.f * Scale, Canvas->SizeX * 0.46f);
	const float X = Canvas->SizeX - Width - 22.f * Scale;
	const float Y = 265.f * Scale;
	const int32 Rows = FMath::Max(1, FMath::DivideAndRoundUp(Owned.Num(), 2));
	const float Height = (49.f + Rows * 34.f) * Scale;
	DrawPanel(X, Y, Width, Height, ChopItHUD::Ink, ChopItHUD::Border, 3.f * Scale);
	DrawLabel(FString::Printf(TEXT("ITEMS  %d  |  F6: PROBAR"), Owned.Num()),
		X + 12.f * Scale, Y + 9.f * Scale, ChopItHUD::Cream, 0.82f * Scale, true);
	if (Owned.IsEmpty())
	{
		DrawLabel(TEXT("SIN ITEMS"), X + 14.f * Scale, Y + 42.f * Scale,
			ChopItHUD::Cream, 0.72f * Scale);
		return;
	}
	const float CellWidth = (Width - 28.f * Scale) * 0.5f;
	for (int32 Index = 0; Index < Owned.Num(); ++Index)
	{
		const UChopItItemDataAsset* Item = Owned[Index].Item;
		const float CellX = X + 14.f * Scale + (Index % 2) * CellWidth;
		const float CellY = Y + (43.f + (Index / 2) * 34.f) * Scale;
		if (UTexture2D* Icon = ResolveItemIcon(Item); Icon && Icon->GetResource())
		{
			FCanvasTileItem Tile(FVector2D(CellX, CellY), Icon->GetResource(), FVector2D(26.f * Scale), FLinearColor::White);
			Tile.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Tile);
		}
		DrawLabel(Item->DisplayName.ToString().Left(15), CellX + 31.f * Scale, CellY + 2.f * Scale,
			ChopItHUD::Cream, 0.68f * Scale);
		DrawLabel(FString::Printf(TEXT("x%d"), Owned[Index].Stacks), CellX + CellWidth - 37.f * Scale,
			CellY + 2.f * Scale, ChopItHUD::Green, 0.68f * Scale);
	}
}

void AChopItHUD::DrawDebugItemMenu(const float Scale)
{
	ChopItHUD::DrawSolidRect(Canvas, 0.f, 0.f, Canvas->SizeX, Canvas->SizeY, FLinearColor(0.f, 0.f, 0.f, 0.70f));
	const float Width = FMath::Min(960.f * Scale, Canvas->SizeX * 0.92f);
	const float Height = FMath::Min(620.f * Scale, Canvas->SizeY * 0.88f);
	const float X = (Canvas->SizeX - Width) * 0.5f;
	const float Y = (Canvas->SizeY - Height) * 0.5f;
	DrawPanel(X, Y, Width, Height, ChopItHUD::Ink, ChopItHUD::Orange, 5.f * Scale);
	DrawCenteredLabel(TEXT("PROBAR ITEMS  [F6 / ESC CERRAR]"), Canvas->SizeX * 0.5f,
		Y + 18.f * Scale, ChopItHUD::Cream, 1.0f * Scale, true);
	if (DebugItemCatalog.IsEmpty())
	{
		DrawCenteredLabel(DebugItemStatus, Canvas->SizeX * 0.5f, Y + Height * 0.5f,
			ChopItHUD::Red, 0.9f * Scale);
		return;
	}
	constexpr int32 PageSize = 10;
	const int32 Page = DebugItemSelection / PageSize;
	const int32 Start = Page * PageSize;
	const float RowHeight = (Height - 145.f * Scale) / PageSize;
	const float ListWidth = Width * 0.58f;
	for (int32 Index = Start; Index < FMath::Min(Start + PageSize, DebugItemCatalog.Num()); ++Index)
	{
		UChopItItemDataAsset* Item = DebugItemCatalog[Index];
		const float RowY = Y + 70.f * Scale + (Index - Start) * RowHeight;
		if (Index == DebugItemSelection)
			ChopItHUD::DrawSolidRect(Canvas, X + 15.f * Scale, RowY - 3.f * Scale,
				ListWidth - 25.f * Scale, RowHeight - 2.f * Scale, ChopItHUD::Orange.CopyWithNewOpacity(0.25f));
		if (UTexture2D* Icon = ResolveItemIcon(Item); Icon && Icon->GetResource())
		{
			FCanvasTileItem Tile(FVector2D(X + 25.f * Scale, RowY), Icon->GetResource(), FVector2D(31.f * Scale), FLinearColor::White);
			Tile.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Tile);
		}
		DrawLabel(Item->DisplayName.ToString().Left(28), X + 67.f * Scale, RowY + 5.f * Scale,
			ChopItHUD::Cream, 0.76f * Scale);
	}
	UChopItItemDataAsset* Selected = DebugItemCatalog[DebugItemSelection];
	const float DetailX = X + ListWidth + 10.f * Scale;
	DrawPanel(DetailX, Y + 72.f * Scale, Width - ListWidth - 25.f * Scale,
		Height - 155.f * Scale, FLinearColor(0.11f, 0.09f, 0.09f, 0.96f), ChopItHUD::Border, 2.f * Scale);
	if (UTexture2D* Icon = ResolveItemIcon(Selected); Icon && Icon->GetResource())
	{
		const float IconSize = 106.f * Scale;
		FCanvasTileItem Tile(FVector2D(DetailX + (Width - ListWidth - 25.f * Scale - IconSize) * 0.5f,
			Y + 92.f * Scale), Icon->GetResource(), FVector2D(IconSize), FLinearColor::White);
		Tile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Tile);
	}
	DrawCenteredLabel(Selected->DisplayName.ToString(), DetailX + (Width - ListWidth - 25.f * Scale) * 0.5f,
		Y + 210.f * Scale, ChopItHUD::Cream, 0.82f * Scale, true);
	const UChopItItemComponent* Inventory = PlayerOwner && PlayerOwner->GetPawn()
		? PlayerOwner->GetPawn()->FindComponentByClass<UChopItItemComponent>() : nullptr;
	DrawLabel(FString::Printf(TEXT("STACKS: %d"), Inventory ? Inventory->GetStackCount(Selected->ItemId) : 0),
		DetailX + 15.f * Scale, Y + 259.f * Scale, ChopItHUD::Green, 0.78f * Scale);
	if (Selected->Effects.IsEmpty())
		DrawLabel(TEXT("EFECTO SIN CONFIGURAR"), DetailX + 15.f * Scale, Y + 289.f * Scale,
			ChopItHUD::Orange, 0.68f * Scale);
	TArray<FString> Words;
	Selected->Description.ToString().ParseIntoArrayWS(Words);
	FString Line;
	float TextY = Y + 325.f * Scale;
	for (const FString& Word : Words)
	{
		const FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
		float TextWidth = 0.f, TextHeight = 0.f;
		Canvas->StrLen(GEngine->GetMediumFont(), Candidate, TextWidth, TextHeight);
		if (!Line.IsEmpty() && TextWidth * 0.68f * Scale > Width - ListWidth - 55.f * Scale)
		{
			DrawLabel(Line, DetailX + 15.f * Scale, TextY, ChopItHUD::Cream, 0.68f * Scale);
			TextY += 23.f * Scale;
			Line = Word;
		}
		else Line = Candidate;
	}
	if (!Line.IsEmpty()) DrawLabel(Line, DetailX + 15.f * Scale, TextY, ChopItHUD::Cream, 0.68f * Scale);
	DrawCenteredLabel(FString::Printf(TEXT("%d / %d   |   PAGINA %d / %d"), DebugItemSelection + 1,
		DebugItemCatalog.Num(), Page + 1, FMath::DivideAndRoundUp(DebugItemCatalog.Num(), PageSize)),
		X + ListWidth * 0.5f, Y + Height - 60.f * Scale, ChopItHUD::Cream, 0.76f * Scale);
	DrawCenteredLabel(TEXT("↑ ↓ ELEGIR   |   RE PÁG / AV PÁG   |   ENTER +1 STACK"),
		Canvas->SizeX * 0.5f, Y + Height - 32.f * Scale, ChopItHUD::Orange, 0.76f * Scale);
	if (!DebugItemStatus.IsEmpty())
		DrawCenteredLabel(DebugItemStatus, Canvas->SizeX * 0.5f, Y + Height - 92.f * Scale,
			ChopItHUD::Green, 0.73f * Scale);
}

float AChopItHUD::GetItemRevealDuration() const
{
	const int32 Tier = ItemRevealResult ? FMath::Clamp(static_cast<int32>(ItemRevealResult->Rarity), 0, 3) : 0;
	return 0.45f + 0.82f * (Tier + 1) + 0.55f;
}

void AChopItHUD::StartItemReveal(UChopItItemDataAsset* Result)
{
	if (!Result) return;
	if (bDebugItemMenuOpen) CloseDebugItemMenu();
	if (bItemRevealActive) CancelItemReveal();
	ItemRevealResult = Result;
	FActorSpawnParameters PreviewParams;
	PreviewParams.ObjectFlags |= RF_Transient;
	PreviewParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ChestRevealScene = GetWorld()->SpawnActor<AChopItChestRevealScene>(AChopItChestRevealScene::StaticClass(), FVector(0,0,100000), FRotator::ZeroRotator, PreviewParams);
	if (ChestRevealScene) ChestRevealScene->InitializeScene();
	ItemRevealIcon = ResolveItemIcon(Result);
	ItemRevealTickSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/ChopIt/UI/Audio/UI_Tick.UI_Tick"));
	ItemRevealStartedAt = FPlatformTime::Seconds();
	LastRevealStage = INDEX_NONE;
	bItemRevealGranted = false;
	bItemRevealActive = true;
	if (PSXHUDWidget) PSXHUDWidget->SetVisibility(ESlateVisibility::Hidden);
	PauseForItemUI();
}

void AChopItHUD::CompleteItemReveal(UChopItItemDataAsset* Result)
{
	if (bItemRevealActive && Result == ItemRevealResult) bItemRevealGranted = true;
}

void AChopItHUD::CancelItemReveal()
{
	if (IsValid(ChestRevealScene)) ChestRevealScene->Destroy();
	ChestRevealScene = nullptr;
	bItemRevealActive = false;
	bItemRevealGranted = false;
	ItemRevealResult = nullptr;
	ItemRevealIcon = nullptr;
	ItemRevealTickSound = nullptr;
	if (PSXHUDWidget) PSXHUDWidget->SetVisibility(ESlateVisibility::Visible);
	ResumeFromItemUI();
}

void AChopItHUD::DismissItemReveal()
{
	if (bItemRevealActive && bItemRevealGranted
		&& FPlatformTime::Seconds() - ItemRevealStartedAt >= GetItemRevealDuration() + 0.65)
	{
		CancelItemReveal();
	}
}

void AChopItHUD::DrawItemReveal(const float Scale)
{
	if (!Canvas || !ItemRevealResult) { CancelItemReveal(); return; }
	const float Elapsed = static_cast<float>(FPlatformTime::Seconds() - ItemRevealStartedAt);
	const float Duration = GetItemRevealDuration();
	const int32 Tier = FMath::Clamp(static_cast<int32>(ItemRevealResult->Rarity), 0, 3);
	const FLinearColor Colors[4] = {
		FLinearColor(0.69f, 0.70f, 0.73f),
		FLinearColor(0.25f, 0.88f, 0.43f),
		FLinearColor(0.25f, 0.55f, 1.0f),
		FLinearColor(1.0f, 0.69f, 0.15f)
	};
	const TCHAR* RarityNames[4] = {TEXT("COMÚN"), TEXT("POCO COMÚN"), TEXT("RARO"), TEXT("LEGENDARIO")};
	const float CX = Canvas->SizeX * 0.5f;
	const float CY = Canvas->SizeY * 0.5f;
	ChopItHUD::DrawSolidRect(Canvas, 0.f, 0.f, Canvas->SizeX, Canvas->SizeY,
		FLinearColor(0.015f, 0.012f, 0.025f, 0.88f));

	if (IsValid(ChestRevealScene) && Elapsed < Duration + 0.65f)
	{
		ChestRevealScene->UpdatePresentation(Elapsed, Duration, Tier);
		if (UTextureRenderTarget2D* Texture = ChestRevealScene->GetTexture())
		{
			const float Size = FMath::Min(Canvas->SizeY * 0.9f, 820.f * Scale);
			FCanvasTileItem Portrait(FVector2D(CX-Size*.5f,CY-Size*.5f), Texture->GetResource(), FVector2D(Size), FLinearColor::White);
			Portrait.BlendMode = SE_BLEND_Opaque;
			Canvas->DrawItem(Portrait);
		}
	}

	if (Elapsed >= Duration && bItemRevealGranted)
	{
		const float Reveal = FMath::Clamp((Elapsed - Duration) / 0.65f, 0.f, 1.f);
		const float CardReveal = FMath::Clamp((Reveal - 0.4f) / 0.6f, 0.f, 1.f);
		const float Zoom = ChopItHUD::EaseOutBack(CardReveal);
		const FLinearColor Color = Colors[Tier];
		const float Pulse = 0.5f + 0.5f * FMath::Sin(Elapsed * 4.f);
		for (int32 Ray = 0; Ray < 16; ++Ray)
		{
			const float Angle = Ray * (2.f * PI / 16.f) + Elapsed * 0.16f;
			const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
			FCanvasLineItem Beam(FVector2D(CX, CY) + Direction * (105.f * Scale),
				FVector2D(CX, CY) + Direction * ((260.f + 35.f * Pulse) * Scale));
			Beam.SetColor(Color.CopyWithNewOpacity(0.13f + 0.12f * Pulse));
			Beam.LineThickness = (6.f + 4.f * Pulse) * Scale;
			Canvas->DrawItem(Beam);
		}
		if (Reveal < 0.4f)
		{
			const float Rise = Reveal / 0.4f;
			const float RisingSize = FMath::Lerp(68.f, 148.f, Rise) * Scale;
			if (ItemRevealIcon && ItemRevealIcon->GetResource())
			{
				FCanvasTileItem RisingIcon(FVector2D(CX - RisingSize * 0.5f,
					CY + FMath::Lerp(96.f, -106.f, Rise) * Scale - RisingSize * 0.5f),
					ItemRevealIcon->GetResource(), FVector2D(RisingSize), FLinearColor::White);
				RisingIcon.BlendMode = SE_BLEND_Translucent;
				Canvas->DrawItem(RisingIcon);
			}
			return;
		}
		const float Width = FMath::Min(FMath::Lerp(260.f, 540.f, Zoom) * Scale, Canvas->SizeX * 0.9f);
		const float Height = FMath::Min(FMath::Lerp(260.f, 500.f, Zoom) * Scale, Canvas->SizeY * 0.88f);
		const float X = CX - Width * 0.5f;
		const float Y = CY - Height * 0.5f;
		DrawPanel(X - 7.f * Scale, Y - 7.f * Scale, Width + 14.f * Scale, Height + 14.f * Scale,
			Color.CopyWithNewOpacity(0.16f), Color.CopyWithNewOpacity(0.5f), 3.f * Scale);
		DrawPanel(X, Y, Width, Height, FLinearColor(0.045f, 0.034f, 0.052f, 0.98f), Color, 5.f * Scale);
		const float Alpha = CardReveal;
		const float IconSize = 148.f * Scale;
		if (ItemRevealIcon && ItemRevealIcon->GetResource())
		{
			FCanvasTileItem Icon(FVector2D(CX - IconSize * 0.5f, Y + 38.f * Scale),
				ItemRevealIcon->GetResource(), FVector2D(IconSize), FLinearColor(1.f, 1.f, 1.f, Alpha));
			Icon.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Icon);
		}
		DrawCenteredLabel(TEXT("RECOMPENSA OBTENIDA"), CX, Y + 14.f * Scale,
			ChopItHUD::WithAlpha(Color, Alpha), 0.78f * Scale);
		DrawCenteredLabel(ItemRevealResult->DisplayName.ToString().ToUpper(), CX, Y + 195.f * Scale,
			ChopItHUD::WithAlpha(ChopItHUD::Cream, Alpha), 1.12f * Scale, true);
		DrawCenteredLabel(RarityNames[Tier], CX, Y + 239.f * Scale,
			ChopItHUD::WithAlpha(Color, Alpha), 0.77f * Scale);
		TArray<FString> Words;
		ItemRevealResult->Description.ToString().ParseIntoArrayWS(Words);
		FString Line;
		float TextY = Y + 287.f * Scale;
		for (const FString& Word : Words)
		{
			const FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
			float TextWidth = 0.f, TextHeight = 0.f;
			Canvas->StrLen(GEngine->GetMediumFont(), Candidate, TextWidth, TextHeight);
			if (!Line.IsEmpty() && TextWidth * 0.70f * Scale > Width - 45.f * Scale)
			{
				DrawCenteredLabel(Line, CX, TextY, ChopItHUD::WithAlpha(ChopItHUD::Cream, Alpha), 0.70f * Scale);
				TextY += 25.f * Scale;
				Line = Word;
			}
			else Line = Candidate;
		}
		if (!Line.IsEmpty()) DrawCenteredLabel(Line, CX, TextY, ChopItHUD::WithAlpha(ChopItHUD::Cream, Alpha), 0.70f * Scale);
		const UChopItItemComponent* Inventory = PlayerOwner && PlayerOwner->GetPawn()
			? PlayerOwner->GetPawn()->FindComponentByClass<UChopItItemComponent>() : nullptr;
		DrawCenteredLabel(FString::Printf(TEXT("STACKS: %d"), Inventory ? Inventory->GetStackCount(ItemRevealResult->ItemId) : 1),
			CX, Y + Height - 75.f * Scale, ChopItHUD::WithAlpha(Color, Alpha), 0.79f * Scale);
		DrawCenteredLabel(TEXT("ENTER / ESC PARA CONTINUAR"), CX, Y + Height - 37.f * Scale,
			ChopItHUD::WithAlpha(ChopItHUD::Cream, Alpha), 0.72f * Scale);
		return;
	}

	const int32 Stage = FMath::Clamp(FMath::FloorToInt((Elapsed - 0.45f) / 0.82f), 0, Tier);
	const float StageTime = FMath::Max(0.f, Elapsed - 0.45f - Stage * 0.82f);
	const float StagePulse = 0.5f + 0.5f * FMath::Sin(StageTime * 10.f);
	const FLinearColor Color = Colors[Stage];
	if (Stage != LastRevealStage && Elapsed >= 0.45f)
	{
		LastRevealStage = Stage;
		if (ItemRevealTickSound) UGameplayStatics::PlaySound2D(this, ItemRevealTickSound, 0.5f + Stage * 0.13f);
	}
	DrawCenteredLabel(TEXT("BOTÍN DEL BOSS"), CX, CY - 265.f * Scale,
		ChopItHUD::Cream, 1.45f * Scale, true);
	const AChopItCharacter* Character = PlayerOwner ? Cast<AChopItCharacter>(PlayerOwner->GetPawn()) : nullptr;
	DrawCenteredLabel(FString::Printf(TEXT("SUERTE: %.0f%%"), Character ? Character->GetLuckPercent() : 0.f),
		CX, CY - 236.f * Scale, ChopItHUD::Cream, 0.68f * Scale);
	for (int32 Pip = 0; Pip < 4; ++Pip)
	{
		const float PX = CX + (Pip - 1.5f) * 72.f * Scale;
		const FLinearColor PipColor = Pip <= Stage ? Colors[Pip] : FLinearColor(0.16f, 0.16f, 0.18f);
		DrawPanel(PX - 18.f * Scale, CY - 210.f * Scale, 36.f * Scale, 20.f * Scale,
			PipColor.CopyWithNewOpacity(Pip <= Stage ? 0.7f : 0.5f), PipColor, 2.f * Scale);
	}
	DrawCenteredLabel(FString::Printf(TEXT("LUZ %s"), RarityNames[Stage]), CX,
		CY + 230.f * Scale, Color, 1.0f * Scale, true);
}

void AChopItHUD::RefreshPSXWidget()
{
	if (Cast<UChopItJuiceWidget>(PSXHUDWidget)) return;
	if (!PSXHUDWidget || !PSXHUDWidget->WidgetTree || !PlayerOwner) return;
	auto Text = [this](const TCHAR* Name) { return Cast<UTextBlock>(PSXHUDWidget->WidgetTree->FindWidget(Name)); };
	auto ImageFill = [this](const TCHAR* Name, const float Fraction, const float FullWidth, const FVector4& AtlasCrop)
	{
		if (UImage* Image = Cast<UImage>(PSXHUDWidget->WidgetTree->FindWidget(Name)))
		{
			const float ClampedFraction = FMath::Clamp(Fraction, 0.f, 1.f);
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Image->Slot))
			{
				const FVector2D CurrentSize = Slot->GetSize();
				// Capture the designer's full width once, before applying any percentage.
				float* DesignWidth = PSXFillDesignWidths.Find(FName(Name));
				if (!DesignWidth)
				{
					DesignWidth = &PSXFillDesignWidths.Add(FName(Name), CurrentSize.X);
				}
				Slot->SetSize(FVector2D(*DesignWidth * ClampedFraction, CurrentSize.Y));
			}
			// Crop the source together with the slot. This reveals the texture instead of
			// squeezing the complete sprite into a progressively smaller rectangle.
			FSlateBrush Brush = Image->GetBrush();
			const double MinX = AtlasCrop.X / 1774.0;
			const double MinY = AtlasCrop.Y / 887.0;
			const double MaxX = (AtlasCrop.X + AtlasCrop.Z * ClampedFraction) / 1774.0;
			const double MaxY = (AtlasCrop.Y + AtlasCrop.W) / 887.0;
			Brush.SetUVRegion(FBox2d(FVector2d(MinX, MinY), FVector2d(MaxX, MaxY)));
			Image->SetBrush(Brush);
		}
	};
	const AChopItGameState* GS = GetWorld() ? GetWorld()->GetGameState<AChopItGameState>() : nullptr;
	const APlayerState* PS = PlayerOwner->PlayerState;
	const AChopItCharacter* Character = Cast<AChopItCharacter>(PlayerOwner->GetPawn());
	if (!GS || !PS || !Character) return;
	const UChopItHealthComponent* Health = Character->GetHealthComponent();
	const UChopItWoodCargoComponent* Cargo = Character->GetWoodCargoComponent();
	const UChopItRunStateComponent* Run = GS->GetRunStateComponent();
	const UChopItCycleStateMachineComponent* Cycle = GS->GetCycleStateMachine();
	const UChopItQuotaComponent* Quota = GS->GetQuotaComponent();
	const UChopItExperienceComponent* XP = PS->FindComponentByClass<UChopItExperienceComponent>();
	const UChopItEconomyComponent* Economy = PS->FindComponentByClass<UChopItEconomyComponent>();
	const float HP = Health ? Health->GetCurrentHealth() : 0.f, MaxHP = Health ? Health->GetMaxHealth() : 1.f;
	if (UTextBlock* W = Text(TEXT("HealthText"))) W->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), HP, MaxHP)));
	ImageFill(TEXT("HealthFill"), HP / FMath::Max(1.f, MaxHP), 180.f, FVector4(99, 255, 747, 35));
	const int32 Q = Quota ? Quota->GetProgress() : 0, QMax = Quota ? Quota->GetTarget() : 1;
	if (UTextBlock* W = Text(TEXT("QuotaText"))) W->SetText(FText::FromString(FString::Printf(TEXT("QUOTA   %d / %d"), Q, QMax)));
	ImageFill(TEXT("QuotaFill"), static_cast<float>(Q) / FMath::Max(1, QMax), 245.f, FVector4(905, 325, 775, 110));
	if (UTextBlock* W = Text(TEXT("WoodText"))) W->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Cargo ? Cargo->GetCurrentWood() : 0, Cargo ? Cargo->GetCapacity() : 0)));
	if (UTextBlock* W = Text(TEXT("MoneyText"))) W->SetText(FText::FromString(FString::Printf(TEXT("$ %lld"), Economy ? Economy->GetBalance() : 0)));
	const int32 Level = XP ? XP->GetLevel() : 1, XPNow = XP ? XP->GetCurrentExperience() : 0, XPMax = XP ? XP->GetRequiredExperience() : 1;
	if (UTextBlock* W = Text(TEXT("LevelText"))) W->SetText(FText::FromString(FString::Printf(TEXT("LEVEL %d"), Level)));
	if (UTextBlock* W = Text(TEXT("XPText"))) W->SetText(FText::FromString(FString::Printf(TEXT("XP %d / %d"), XPNow, XPMax)));
	ImageFill(TEXT("XPFill"), static_cast<float>(XPNow) / FMath::Max(1, XPMax), 1300.f, FVector4(284, 113, 1365, 35));
	const int32 Seconds = FMath::Max(0, FMath::RoundToInt(Cycle ? Cycle->GetPhaseRemaining() : 0.f));
	if (UTextBlock* W = Text(TEXT("TimeText"))) W->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Seconds / 60, Seconds % 60)));
	if (UTextBlock* W = Text(TEXT("DayText"))) W->SetText(FText::FromString(FString::Printf(TEXT("DAY %d"), Run ? Run->GetDayNumber() : 1)));
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
