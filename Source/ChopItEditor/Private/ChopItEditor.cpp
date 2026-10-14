// Editor module: terrain tab registration and level editing actions.
#include "Modules/ModuleManager.h"
#include "Terrain/ChopItTerrainActor.h"
#include "Terrain/ChopItTerrainEditorActions.h"
#include "Terrain/ChopItTerrainEditorSettings.h"
#include "Terrain/ChopItTerrainParameterTools.h"
#include "Terrain/ChopItTerrainPresets.h"
#include "Terrain/ChopItTerrainStatistics.h"
#include "Terrain/ChopItTerrainDataAsset.h"
#include "Terrain/ChopItTerrainDetailsCustomization.h"
#include "Terrain/ChopItTerrainAssetCustomization.h"
#include "Terrain/SChopItTerrainPreviewViewport.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "Framework/Docking/TabManager.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "ChopItTerrainEditor"

namespace
{
const FName TerrainTabId(TEXT("ChopItTerrainGenerator"));

struct FTerrainTabState
{
	TWeakObjectPtr<AChopItTerrainActor> Target;
	FString Status = TEXT("Choose a terrain actor or press Generate to create one.");
	FString Statistics = TEXT("No generated terrain data.");
};

AChopItTerrainActor* GetSelectedTerrain()
{
	if (!GEditor || !GEditor->GetSelectedActors()) return nullptr;
	for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
	{
		if (AChopItTerrainActor* Terrain = Cast<AChopItTerrainActor>(*It)) return Terrain;
	}
	return nullptr;
}
}

class FChopItEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"))
			.RegisterCustomClassLayout(UChopItTerrainDataAsset::StaticClass()->GetFName(),
				FOnGetDetailCustomizationInstance::CreateStatic(&FChopItTerrainAssetCustomization::MakeInstance));
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TerrainTabId,
			FOnSpawnTab::CreateRaw(this, &FChopItEditorModule::SpawnTerrainTab))
			.SetDisplayName(LOCTEXT("TabTitle", "ChopIt Terrain Generator"))
			.SetTooltipText(LOCTEXT("TabTooltip", "Generate a forest terrain in the open editor level."));
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FChopItEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		if (FPropertyEditorModule* PropertyEditor = FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
			PropertyEditor->UnregisterCustomClassLayout(UChopItTerrainDataAsset::StaticClass()->GetFName());
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TerrainTabId);
	}

private:
	void RegisterMenus()
	{
		FToolMenuOwnerScoped Owner(this);
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
		FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("ChopIt"));
		Section.AddEntry(FToolMenuEntry::InitMenuEntry(TerrainTabId,
			LOCTEXT("OpenTerrain", "ChopIt Terrain Generator"),
			LOCTEXT("OpenTerrainTip", "Open the dockable forest terrain generator."),
			FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([]()
			{
				FGlobalTabmanager::Get()->TryInvokeTab(TerrainTabId);
			}))));
	}

	TSharedRef<SDockTab> SpawnTerrainTab(const FSpawnTabArgs& Args)
	{
		UChopItTerrainEditorSettings* Settings = GetMutableDefault<UChopItTerrainEditorSettings>();
		TSharedRef<FTerrainTabState> State = MakeShared<FTerrainTabState>();
		State->Target = GetSelectedTerrain();
		if (AChopItTerrainActor* Target = State->Target.Get())
		{
			Settings->Terrain = Target->TerrainSettings;
			Settings->Mesh = Target->MeshSettings;
			Settings->BrushWorldPosition = Target->GetActorLocation();
		}
		State->Statistics = FChopItTerrainStatistics::Describe(State->Target.Get());

		FDetailsViewArgs ViewArgs;
		ViewArgs.bAllowSearch = false;
		ViewArgs.bHideSelectionTip = true;
		ViewArgs.bShowOptions = false;
		ViewArgs.bShowPropertyMatrixButton = false;
		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		TSharedRef<IDetailsView> Details = PropertyEditor.CreateDetailView(ViewArgs);
		Details->RegisterInstancedCustomPropertyLayout(UChopItTerrainEditorSettings::StaticClass(),
			FOnGetDetailCustomizationInstance::CreateLambda([Settings]() -> TSharedRef<IDetailCustomization>
			{
				return MakeShared<FChopItTerrainDetailsCustomization>(Settings);
			}));
		const auto RegisterTerrainType = [Details, Settings](FName Type, FString Prefix)
		{
			Details->RegisterInstancedCustomPropertyTypeLayout(Type,
				FOnGetPropertyTypeCustomizationInstance::CreateLambda([Settings, Prefix]() -> TSharedRef<IPropertyTypeCustomization>
				{
					return MakeShared<FChopItTerrainPropertyTypeCustomization>(Settings, Prefix);
				}));
		};
		RegisterTerrainType(FChopItTerrainSettings::StaticStruct()->GetFName(), TEXT("Terrain."));
		RegisterTerrainType(FChopItTerrainMeshSettings::StaticStruct()->GetFName(), TEXT("Mesh."));
		RegisterTerrainType(FChopItTerrainRegionSettings::StaticStruct()->GetFName(), TEXT("Terrain.Regions."));
		RegisterTerrainType(FChopItTerrainNoiseLayer::StaticStruct()->GetFName(), TEXT("NoiseLayer."));
		TSharedRef<TWeakPtr<SChopItTerrainPreviewViewport>> PreviewHandle = MakeShared<TWeakPtr<SChopItTerrainPreviewViewport>>();
		Details->SetObject(Settings);
		Details->OnFinishedChangingProperties().AddLambda([Settings](const FPropertyChangedEvent&)
		{
			Settings->SaveConfig();
		});
		TSharedPtr<SChopItTerrainPreviewViewport> PreviewWidget;
		SAssignNew(PreviewWidget, SChopItTerrainPreviewViewport).Settings(Settings)
		.OnTerrainClick([State, Settings, PreviewHandle](const FVector2D& XY)
		{
			AChopItTerrainActor* Target = State->Target.Get();
			if (!IsValid(Target)) { State->Status = TEXT("Select a terrain actor before painting the preview."); return; }
			if (!FChopItTerrainSettings::StaticStruct()->CompareScriptStruct(&Settings->Terrain, &Target->GetGeneratedSettings(), 0))
			{ State->Status = TEXT("Regenerate the selected terrain before painting these preview settings."); return; }
			const FVector World = Target->GetActorTransform().TransformPosition(FVector(XY.X, XY.Y, 0));
			Settings->BrushWorldPosition = World;
			Settings->SaveConfig();
			FString Error;
			const FScopedTransaction Transaction(LOCTEXT("PreviewMaskUndo", "Paint ChopIt Mask in Preview"));
			if (Target->ApplyWorldMaskBrush(Settings->EditMask, Settings->PreviewBrush, World,
				Settings->BrushRadius, Settings->BrushStrength, Settings->BrushTargetValue, Error))
			{
				if (TSharedPtr<SChopItTerrainPreviewViewport> Widget = PreviewHandle->Pin()) Widget->ShowActorMasks(Target->GetHeightField());
				State->Status = TEXT("Mask edited in preview. Save the level to keep it.");
				State->Statistics = FChopItTerrainStatistics::Describe(Target);
			}
			else State->Status = Error;
		});
		*PreviewHandle = PreviewWidget;
		PreviewWidget->SetMaskActor(State->Target.Get());

		return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SBorder).Padding(8)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
				[
					SNew(STextBlock).Text_Lambda([State]()
					{
						return FText::FromString(IsValid(State->Target.Get())
							? FString::Printf(TEXT("Target: %s"), *State->Target->GetActorLabel())
							: TEXT("Target: new terrain"));
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
				[
					SNew(SButton).Text(LOCTEXT("UseSelected", "Use Selected Terrain"))
					.ToolTipText(LOCTEXT("UseSelectedTip", "Bind this window to the selected ChopIt terrain actor."))
					.OnClicked_Lambda([State, Settings, Details, PreviewHandle]()
					{
						State->Target = GetSelectedTerrain();
						if (AChopItTerrainActor* Target = State->Target.Get())
						{
							Settings->Terrain = Target->TerrainSettings;
							Settings->Mesh = Target->MeshSettings;
							Settings->BrushWorldPosition = Target->GetActorLocation();
							Settings->SaveConfig();
							Details->ForceRefresh();
						}
						if (TSharedPtr<SChopItTerrainPreviewViewport> Widget = PreviewHandle->Pin()) Widget->SetMaskActor(State->Target.Get());
						State->Statistics = FChopItTerrainStatistics::Describe(State->Target.Get());
						State->Status = State->Target.IsValid() ? TEXT("Existing terrain selected.") : TEXT("Select a ChopIt terrain actor in the level.");
						return FReply::Handled();
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
				[
					SNew(SExpandableArea)
					.AreaTitle(LOCTEXT("StylesAndParameters", "Presets and Parameter Tools"))
					.InitiallyCollapsed(true)
					.BodyContent()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(2)
						[
							SNew(SSearchBox)
							.HintText(LOCTEXT("FindParameter", "Search terrain parameters..."))
							.OnTextChanged_Lambda([Details](const FText& Query) { Details->SetSearchText(Query); })
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(2)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("ApplyPreset", "Apply Preset"))
							.ToolTipText(LOCTEXT("ApplyPresetTip", "Apply the selected forest style; locked values and the current seed and dimensions are kept."))
							.OnClicked_Lambda([State, Settings, Details]()
							{
								FChopItTerrainSettings Terrain; FChopItTerrainMeshSettings Mesh; FString Error;
								const bool bDone = FChopItTerrainPresets::Build(Settings->SelectedPreset, Settings->Terrain, Terrain, Mesh, Error)
									&& FChopItTerrainParameterTools::ApplyUnlocked(*Settings, Terrain, Settings->Mesh, Error);
								if (bDone) Details->ForceRefresh();
								State->Status = bDone ? TEXT("Preset applied to unlocked controls. Press Generate to use it.") : Error;
								return FReply::Handled();
							})]
							+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("SavePreset", "Save Preset"))
							.ToolTipText(LOCTEXT("SavePresetTip", "Save current terrain and mesh controls to the Presets > Preset File path."))
							.OnClicked_Lambda([State, Settings]()
							{
								FString Error;
								State->Status = FChopItTerrainPresets::Save(Settings->PresetFile.FilePath, Settings->Terrain, Settings->Mesh, Error)
									? TEXT("Preset saved.") : Error;
								return FReply::Handled();
							})]
							+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("LoadPreset", "Load Preset"))
							.ToolTipText(LOCTEXT("LoadPresetTip", "Load controls from the Presets > Preset File path while preserving locked values."))
							.OnClicked_Lambda([State, Settings, Details]()
							{
								FChopItTerrainSettings Terrain; FChopItTerrainMeshSettings Mesh; FString Error;
								const bool bDone = FChopItTerrainPresets::Load(Settings->PresetFile.FilePath, Terrain, Mesh, Error)
									&& FChopItTerrainParameterTools::ApplyUnlocked(*Settings, Terrain, Mesh, Error);
								if (bDone) Details->ForceRefresh();
								State->Status = bDone ? TEXT("Preset loaded into unlocked controls. Press Generate to use it.") : Error;
								return FReply::Handled();
							})]
							]
						+ SVerticalBox::Slot().AutoHeight().Padding(2)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("RandomAll", "Randomize Settings"))
							.ToolTipText(LOCTEXT("RandomAllTip", "Randomize valid numeric and boolean controls without changing locked parameters."))
							.OnClicked_Lambda([State, Settings, Details]()
							{
								FString Error;
								const bool bDone = FChopItTerrainParameterTools::Randomize(*Settings, TEXT("All"), Error);
								if (bDone) Details->ForceRefresh();
								State->Status = bDone ? TEXT("Settings randomized. Press Generate to use them.") : Error;
								return FReply::Handled();
							})]
							+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("RandomCategory", "Randomize Category"))
							.ToolTipText(LOCTEXT("RandomCategoryTip", "Randomize only the chosen Parameter Tools category."))
							.OnClicked_Lambda([State, Settings, Details]()
							{
								FString Error;
								const bool bDone = FChopItTerrainParameterTools::Randomize(*Settings, Settings->SelectedCategory, Error);
								if (bDone) Details->ForceRefresh();
								State->Status = bDone ? FString::Printf(TEXT("%s randomized. Press Generate to use it."), *Settings->SelectedCategory) : Error;
								return FReply::Handled();
							})]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(2)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton)
							.Text_Lambda([Settings]() { return FText::FromString(FChopItTerrainParameterTools::IsLocked(*Settings, Settings->SelectedParameter)
								? TEXT("Unlock Parameter") : TEXT("Lock Parameter")); })
							.ToolTipText(LOCTEXT("LockTip", "Keep the selected parameter unchanged when applying presets, randomizing or restoring defaults."))
							.OnClicked_Lambda([State, Settings, Details]()
							{
								FString Error;
								const bool bDone = FChopItTerrainParameterTools::ToggleLock(*Settings, Settings->SelectedParameter, Error);
								if (bDone) Details->ForceRefresh();
								State->Status = bDone ? (FChopItTerrainParameterTools::IsLocked(*Settings, Settings->SelectedParameter)
									? TEXT("Parameter locked.") : TEXT("Parameter unlocked.")) : Error;
								return FReply::Handled();
							})]
							+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("ResetParameter", "Reset Parameter"))
							.ToolTipText(LOCTEXT("ResetParameterTip", "Restore only the selected parameter to its original default value."))
							.OnClicked_Lambda([State, Settings, Details]()
							{
								FString Error;
								const bool bDone = FChopItTerrainParameterTools::ResetParameter(*Settings, Settings->SelectedParameter, Error);
								if (bDone) Details->ForceRefresh();
								State->Status = bDone ? TEXT("Parameter restored. Press Generate to use it.") : Error;
								return FReply::Handled();
							})]
							]
						]
				]
				+ SVerticalBox::Slot().FillHeight(1).Padding(0, 0, 0, 8)
				[
					SNew(SSplitter)
					+ SSplitter::Slot().Value(0.4f)[Details]
					+ SSplitter::Slot().Value(0.6f)[PreviewWidget.ToSharedRef()]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 3, 0)
					[
						SNew(SButton).Text(LOCTEXT("Generate", "Generate"))
						.ToolTipText(LOCTEXT("GenerateTip", "Create terrain if none is targeted, or update the current target."))
						.OnClicked_Lambda([State, Settings, PreviewHandle]()
						{
							UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
							AChopItTerrainActor* Target = State->Target.Get();
							FString Error;
							if (FChopItTerrainEditorActions::Generate(World, Target, *Settings, Error))
							{
								State->Target = Target;
								if (TSharedPtr<SChopItTerrainPreviewViewport> Widget = PreviewHandle->Pin()) Widget->SetMaskActor(Target);
								State->Status = TEXT("Terrain generated. Save the level to keep it.");
								State->Statistics = FChopItTerrainStatistics::Describe(Target);
								GEditor->SelectNone(false, true);
								GEditor->SelectActor(Target, true, true);
							}
							else State->Status = Error;
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().FillWidth(1).Padding(3, 0, 0, 0)
					[
						SNew(SButton).Text(LOCTEXT("Regenerate", "Regenerate"))
						.ToolTipText(LOCTEXT("RegenerateTip", "Rebuild the current target with these settings."))
						.OnClicked_Lambda([State, Settings, PreviewHandle]()
						{
							UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
							FString Error;
							const bool bDone = FChopItTerrainEditorActions::Regenerate(World, State->Target.Get(), *Settings, Error);
							if (bDone) if (TSharedPtr<SChopItTerrainPreviewViewport> Widget = PreviewHandle->Pin()) Widget->SetMaskActor(State->Target.Get());
							State->Status = bDone ? TEXT("Terrain regenerated. Save the level to keep it.") : Error;
							if (bDone) State->Statistics = FChopItTerrainStatistics::Describe(State->Target.Get());
							return FReply::Handled();
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 3, 0)
					[
						SNew(SButton).Text(LOCTEXT("RandomSeed", "Random Seed"))
						.OnClicked_Lambda([State, Settings, Details]()
						{
							FString Error;
							const bool bDone = FChopItTerrainEditorActions::RandomSeed(*Settings, Error);
							Details->ForceRefresh();
							State->Status = bDone ? TEXT("Seed changed. Press Generate to apply it.") : Error;
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().FillWidth(1).Padding(3, 0, 0, 0)
					[
						SNew(SButton).Text(LOCTEXT("Reset", "Reset Defaults"))
						.OnClicked_Lambda([State, Settings, Details]()
						{
							FString Error;
							const bool bDone = FChopItTerrainEditorActions::ResetDefaults(*Settings, Error);
							Details->ForceRefresh();
							State->Status = bDone ? TEXT("Defaults restored for unlocked controls. Press Generate to apply them.") : Error;
							return FReply::Handled();
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("PaintMask", "Paint Mask"))
					.OnClicked_Lambda([State, Settings, PreviewHandle]()
					{
						AChopItTerrainActor* Target = State->Target.Get();
						if (!IsValid(Target)) { State->Status = TEXT("Select a terrain actor first."); return FReply::Handled(); }
						const FScopedTransaction Transaction(LOCTEXT("PaintMaskUndo", "Paint ChopIt Mask"));
						FString Error;
						const bool bDone = Target->ApplyWorldMaskBrush(Settings->EditMask, EChopItMaskBrush::Paint,
							Settings->BrushWorldPosition, Settings->BrushRadius, Settings->BrushStrength, Settings->BrushTargetValue, Error);
						if (bDone) if (TSharedPtr<SChopItTerrainPreviewViewport> Widget = PreviewHandle->Pin()) Widget->ShowActorMasks(Target->GetHeightField());
						State->Status = bDone ? TEXT("Mask painted. Save the level to keep the edit.") : Error;
						if (bDone) State->Statistics = FChopItTerrainStatistics::Describe(Target);
						return FReply::Handled();
					})]
					+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("EraseMask", "Erase Mask"))
					.OnClicked_Lambda([State, Settings, PreviewHandle]()
					{
						AChopItTerrainActor* Target = State->Target.Get();
						if (!IsValid(Target)) { State->Status = TEXT("Select a terrain actor first."); return FReply::Handled(); }
						const FScopedTransaction Transaction(LOCTEXT("EraseMaskUndo", "Erase ChopIt Mask"));
						FString Error;
						const bool bDone = Target->ApplyWorldMaskBrush(Settings->EditMask, EChopItMaskBrush::Erase,
							Settings->BrushWorldPosition, Settings->BrushRadius, Settings->BrushStrength, Settings->BrushTargetValue, Error);
						if (bDone) if (TSharedPtr<SChopItTerrainPreviewViewport> Widget = PreviewHandle->Pin()) Widget->ShowActorMasks(Target->GetHeightField());
						State->Status = bDone ? TEXT("Mask erased to procedural values.") : Error;
						if (bDone) State->Statistics = FChopItTerrainStatistics::Describe(Target);
						return FReply::Handled();
					})]
					+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("SmoothMask", "Smooth Mask"))
					.OnClicked_Lambda([State, Settings, PreviewHandle]()
					{
						AChopItTerrainActor* Target = State->Target.Get();
						if (!IsValid(Target)) { State->Status = TEXT("Select a terrain actor first."); return FReply::Handled(); }
						const FScopedTransaction Transaction(LOCTEXT("SmoothMaskUndo", "Smooth ChopIt Mask"));
						FString Error;
						const bool bDone = Target->ApplyWorldMaskBrush(Settings->EditMask, EChopItMaskBrush::Smooth,
							Settings->BrushWorldPosition, Settings->BrushRadius, Settings->BrushStrength, Settings->BrushTargetValue, Error);
						if (bDone) if (TSharedPtr<SChopItTerrainPreviewViewport> Widget = PreviewHandle->Pin()) Widget->ShowActorMasks(Target->GetHeightField());
						State->Status = bDone ? TEXT("Mask smoothed.") : Error;
						if (bDone) State->Statistics = FChopItTerrainStatistics::Describe(Target);
						return FReply::Handled();
					})]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("SaveData", "Save Data"))
					.OnClicked_Lambda([State, Settings]()
					{
						FString Error;
						AChopItTerrainActor* Target = State->Target.Get();
						State->Status = IsValid(Target) && Target->SaveTerrainData(Settings->TerrainDataFile.FilePath, Error)
							? TEXT("Terrain data saved.") : Error.IsEmpty() ? TEXT("Select a terrain actor and set Data > Terrain Data File.") : Error;
						return FReply::Handled();
					})]
					+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("LoadData", "Load Data"))
					.OnClicked_Lambda([State, Settings, Details, PreviewHandle]()
					{
						FString Error;
						AChopItTerrainActor* Target = State->Target.Get();
						if (IsValid(Target))
						{
							const FScopedTransaction Transaction(LOCTEXT("LoadDataUndo", "Load ChopIt Terrain Data"));
							if (Target->LoadTerrainData(Settings->TerrainDataFile.FilePath, Error))
							{
								Settings->Terrain = Target->TerrainSettings;
								Settings->Mesh = Target->MeshSettings;
								Settings->SaveConfig(); Details->ForceRefresh();
								if (TSharedPtr<SChopItTerrainPreviewViewport> Widget = PreviewHandle->Pin()) Widget->SetMaskActor(Target);
								State->Status = TEXT("Terrain data loaded. Save the level to keep it.");
								State->Statistics = FChopItTerrainStatistics::Describe(Target);
								return FReply::Handled();
							}
						}
						State->Status = Error.IsEmpty() ? TEXT("Select a terrain actor and set Data > Terrain Data File.") : Error;
						return FReply::Handled();
					})]
					+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("BakeMesh", "Bake Mesh"))
					.ToolTipText(LOCTEXT("BakeMeshTip", "Create a Static Mesh asset from the generated chunks. Save the new asset to keep it."))
					.OnClicked_Lambda([State]()
					{
						FString Error;
						UStaticMesh* Asset = FChopItTerrainEditorActions::BakeMesh(State->Target.Get(), TEXT("/Game/ChopIt/World/Generated/SM_Terrain"), Error);
						State->Status = Asset ? FString::Printf(TEXT("Baked %s. Save the asset to keep it."), *Asset->GetPathName()) : Error;
						if (Asset) GEditor->SyncBrowserToObjects(TArray<UObject*>{ Asset });
						return FReply::Handled();
					})]
					+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("SaveAsDataAsset", "Save As Data Asset"))
					.ToolTipText(LOCTEXT("SaveAsDataAssetTip", "Save the current terrain and mesh values as a runtime Data Asset. The Seed is supplied by the game."))
					.OnClicked_Lambda([State, Settings]()
					{
						FString Error;
						UChopItTerrainDataAsset* Asset = FChopItTerrainEditorActions::SaveAsDataAsset(*Settings, Error);
						State->Status = Asset ? FString::Printf(TEXT("Saved %s. Supply a seed in game to generate a terrain actor."), *Asset->GetPathName()) : Error;
						if (Asset) GEditor->SyncBrowserToObjects(TArray<UObject*>{ Asset });
						return FReply::Handled();
					})]
					+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(LOCTEXT("ExportMaps", "Export Maps"))
					.OnClicked_Lambda([State, Settings]()
					{
						FString Error;
						AChopItTerrainActor* Target = State->Target.Get();
						State->Status = IsValid(Target) && Target->ExportTerrainMaps(Settings->ExportDirectory.Path, Error)
							? TEXT("Heightmap and masks exported.") : Error.IsEmpty() ? TEXT("Select a terrain actor and set Data > Export Directory.") : Error;
						return FReply::Handled();
					})]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SExpandableArea)
					.AreaTitle(LOCTEXT("TerrainStatistics", "Terrain Statistics"))
					.InitiallyCollapsed(true)
					.BodyContent()
					[
						SNew(STextBlock).AutoWrapText(true)
						.Text_Lambda([State]() { return FText::FromString(State->Statistics); })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
				[
					SNew(STextBlock).AutoWrapText(true).Text_Lambda([State]() { return FText::FromString(State->Status); })
				]
			]
		];
	}
};

IMPLEMENT_MODULE(FChopItEditorModule, ChopItEditor);

#undef LOCTEXT_NAMESPACE
