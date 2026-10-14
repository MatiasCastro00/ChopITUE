#include "Terrain/ChopItTerrainDetailsCustomization.h"
#include "Terrain/ChopItTerrainEditorSettings.h"
#include "Terrain/ChopItTerrainParameterTools.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "IDetailPropertyRow.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

namespace
{
bool EditorControlDiffersFromDefault(const UChopItTerrainEditorSettings& Settings, FName Name)
{
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, PreviewPolygonDetail)) return Settings.PreviewPolygonDetail != 35;
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, PreviewMap)) return Settings.PreviewMap != EChopItTerrainPreviewMode::Surface;
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, EditMask)) return Settings.EditMask != EChopItEditableMask::Clearing;
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, BrushWorldPosition)) return Settings.BrushWorldPosition != FVector::ZeroVector;
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, BrushRadius)) return Settings.BrushRadius != 1000.0f;
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, BrushStrength)) return Settings.BrushStrength != 1.0f;
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, BrushTargetValue)) return Settings.BrushTargetValue != 1.0f;
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, bPaintOnPreview)) return Settings.bPaintOnPreview;
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, PreviewBrush)) return Settings.PreviewBrush != EChopItMaskBrush::Paint;
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, TerrainDataFile)) return !Settings.TerrainDataFile.FilePath.IsEmpty();
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, ExportDirectory)) return !Settings.ExportDirectory.Path.IsEmpty();
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, SelectedPreset)) return Settings.SelectedPreset != EChopItTerrainPreset::FlatForest;
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, PresetFile)) return !Settings.PresetFile.FilePath.IsEmpty();
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, SelectedCategory)) return Settings.SelectedCategory != TEXT("Relief");
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, SelectedParameter)) return Settings.SelectedParameter != TEXT("Terrain.HillStrength");
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, LockedParameters)) return !Settings.LockedParameters.IsEmpty();
	return false;
}

bool ResetEditorControl(UChopItTerrainEditorSettings& Settings, FName Name)
{
	Settings.Modify();
	if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, PreviewPolygonDetail)) Settings.PreviewPolygonDetail = 35;
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, PreviewMap)) Settings.PreviewMap = EChopItTerrainPreviewMode::Surface;
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, EditMask)) Settings.EditMask = EChopItEditableMask::Clearing;
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, BrushWorldPosition)) Settings.BrushWorldPosition = FVector::ZeroVector;
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, BrushRadius)) Settings.BrushRadius = 1000.0f;
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, BrushStrength)) Settings.BrushStrength = 1.0f;
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, BrushTargetValue)) Settings.BrushTargetValue = 1.0f;
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, bPaintOnPreview)) Settings.bPaintOnPreview = false;
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, PreviewBrush)) Settings.PreviewBrush = EChopItMaskBrush::Paint;
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, TerrainDataFile)) Settings.TerrainDataFile.FilePath.Empty();
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, ExportDirectory)) Settings.ExportDirectory.Path.Empty();
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, SelectedPreset)) Settings.SelectedPreset = EChopItTerrainPreset::FlatForest;
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, PresetFile)) Settings.PresetFile.FilePath.Empty();
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, SelectedCategory)) Settings.SelectedCategory = TEXT("Relief");
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, SelectedParameter)) Settings.SelectedParameter = TEXT("Terrain.HillStrength");
	else if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, LockedParameters)) Settings.LockedParameters.Reset();
	else return false;
	Settings.SaveConfig();
	return true;
}

bool NoiseLayerDiffersFromDefault(const TSharedRef<IPropertyHandle>& Handle)
{
	const FProperty* Property = Handle->GetProperty();
	const FChopItTerrainNoiseLayer Defaults;
	const void* DefaultValue = Property->ContainerPtrToValuePtr<void>(&Defaults);
	TArray<const void*> Values;
	Handle->AccessRawData(Values);
	for (const void* Value : Values)
		if (Value && !Property->Identical(Value, DefaultValue)) return true;
	return false;
}

void ResetNoiseLayerField(const TSharedRef<IPropertyHandle>& Handle)
{
	const FProperty* Property = Handle->GetProperty();
	const FChopItTerrainNoiseLayer Defaults;
	FString Text;
	Property->ExportTextItem_Direct(Text, Property->ContainerPtrToValuePtr<void>(&Defaults), nullptr, nullptr, PPF_None);
	Handle->SetValueFromFormattedString(Text);
}

void Decorate(IDetailPropertyRow& Row, TSharedRef<IPropertyHandle> Handle,
	TWeakObjectPtr<UChopItTerrainEditorSettings> Settings, const FString& Path,
	TWeakPtr<IPropertyUtilities> Utilities)
{
	TSharedPtr<SWidget> DefaultName, DefaultValue;
	Row.GetDefaultWidgets(DefaultName, DefaultValue);
	const FProperty* Property = Handle->GetProperty();
	FText Description = Property ? Property->GetToolTipText() : FText::GetEmpty();
	if (Description.IsEmpty()) Description = FText::Format(NSLOCTEXT("ChopItTerrain", "ParameterHelp", "Control for {0}."), Handle->GetPropertyDisplayName());
	const bool bTerrainParameter = Path.StartsWith(TEXT("Terrain.")) || Path.StartsWith(TEXT("Mesh."));
	const bool bNoiseLayer = Path.StartsWith(TEXT("NoiseLayer."));
	const bool bEditorControl = Path.IsEmpty();
	FDetailWidgetRow& Widgets = Row.CustomWidget(true);
	Widgets.NameContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
		[DefaultName.IsValid() ? DefaultName.ToSharedRef() : Handle->CreatePropertyNameWidget()]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("\xf06a")))
			.Font(FAppStyle::GetFontStyle(TEXT("FontAwesome.10")))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.ToolTipText(Description)
		]
	];
	Widgets.ValueContent().MinDesiredWidth(125).MaxDesiredWidth(450)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
		[DefaultValue.IsValid() ? DefaultValue.ToSharedRef() : Handle->CreatePropertyValueWidget()]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(3, 0, 0, 0)
		[
			SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
			.ContentPadding(FMargin(2))
			.ToolTipText(NSLOCTEXT("ChopItTerrain", "ResetThisParameter", "Restore this parameter to its default value."))
			.Visibility_Lambda([Handle, Settings, Path, bTerrainParameter, bNoiseLayer, bEditorControl]()
			{
			if (!Settings.IsValid()) return EVisibility::Collapsed;
			const bool bChanged = bTerrainParameter ? !FChopItTerrainParameterTools::IsDefault(*Settings.Get(), Path)
					: bNoiseLayer ? NoiseLayerDiffersFromDefault(Handle)
					: bEditorControl ? EditorControlDiffersFromDefault(*Settings.Get(), Handle->GetProperty()->GetFName())
					: Handle->DiffersFromDefault();
			return bChanged ? EVisibility::Visible : EVisibility::Collapsed;
		})
			.IsEnabled_Lambda([Settings, Path, bTerrainParameter]()
			{
			return Settings.IsValid() && (!bTerrainParameter || !FChopItTerrainParameterTools::IsLocked(*Settings.Get(), Path));
		})
			.OnClicked_Lambda([Handle, Settings, Path, bTerrainParameter, bNoiseLayer, bEditorControl, Utilities]()
			{
				if (!Settings.IsValid()) return FReply::Handled();
				if (bTerrainParameter)
				{
					FString Error;
					if (!FChopItTerrainParameterTools::ResetParameter(*Settings.Get(), Path, Error))
					{
						FNotificationInfo Info(FText::FromString(Error));
						Info.ExpireDuration = 4;
						FSlateNotificationManager::Get().AddNotification(Info);
					}
					else if (TSharedPtr<IPropertyUtilities> Utility = Utilities.Pin()) Utility->ForceRefresh();
				}
				else if (bNoiseLayer)
				{
					ResetNoiseLayerField(Handle);
					Settings->SaveConfig();
				}
				else if (bEditorControl)
				{
					if (!ResetEditorControl(*Settings.Get(), Handle->GetProperty()->GetFName())) Handle->ResetToDefault();
					if (TSharedPtr<IPropertyUtilities> Utility = Utilities.Pin()) Utility->ForceRefresh();
				}
				else
				{
					Handle->ResetToDefault();
					Settings->SaveConfig();
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("\xf0e2")))
				.Font(FAppStyle::GetFontStyle(TEXT("FontAwesome.10")))
			]
		]
	];
	Row.OverrideResetToDefault(FResetToDefaultOverride::Hide());
}
}

void FChopItTerrainDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	for (TFieldIterator<FProperty> It(UChopItTerrainEditorSettings::StaticClass()); It; ++It)
	{
		const FName Name = It->GetFName();
		if (Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, Terrain)
			|| Name == GET_MEMBER_NAME_CHECKED(UChopItTerrainEditorSettings, Mesh)) continue;
		TSharedRef<IPropertyHandle> Handle = DetailBuilder.GetProperty(Name);
		if (!Handle->IsValidHandle()) continue;
		if (IDetailPropertyRow* Row = DetailBuilder.EditDefaultProperty(Handle))
			Decorate(*Row, Handle, Settings, FString(), DetailBuilder.GetPropertyUtilities());
	}
}

void FChopItTerrainPropertyTypeCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle,
	FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// The Terrain and Mesh structs are already inlined by ShowOnlyInnerProperties.
}

void FChopItTerrainPropertyTypeCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	uint32 Count = 0;
	PropertyHandle->GetNumChildren(Count);
	TMap<FString, IDetailGroup*> Groups;
	for (uint32 Index = 0; Index < Count; ++Index)
	{
		TSharedPtr<IPropertyHandle> Child = PropertyHandle->GetChildHandle(Index);
		if (!Child.IsValid() || !Child->IsValidHandle()) continue;
		const FProperty* ChildProperty = Child->GetProperty();
		const FName Name = ChildProperty->GetFName();
		FString Category = ChildProperty->GetMetaData(TEXT("Category"));
		int32 Separator = INDEX_NONE;
		if (Category.FindLastChar(TEXT('|'), Separator)) Category.RightChopInline(Separator + 1);

		// The outer details category already supplies these headings.
		const bool bInline = Category.IsEmpty()
			|| (Prefix == TEXT("Terrain.") && Category == TEXT("Generation"))
			|| (Prefix == TEXT("Mesh.") && Category == TEXT("Geometry"))
			|| (Prefix == TEXT("Terrain.Regions.") && Category == TEXT("Regions"))
			|| (Prefix == TEXT("NoiseLayer.") && Category == TEXT("Noise Layer"));
		IDetailPropertyRow* Row = nullptr;
		if (bInline)
		{
			Row = &ChildBuilder.AddProperty(Child.ToSharedRef());
		}
		else
		{
			IDetailGroup*& Group = Groups.FindOrAdd(Category);
			if (!Group)
			{
				Group = &ChildBuilder.AddGroup(FName(*(Prefix + Category)), FText::FromString(Category));
			}
			Row = &Group->AddPropertyRow(Child.ToSharedRef());
		}
		if (Name == GET_MEMBER_NAME_CHECKED(FChopItTerrainSettings, Regions)) continue;
		Decorate(*Row, Child.ToSharedRef(), Settings, Prefix + Name.ToString(), CustomizationUtils.GetPropertyUtilities());
	}
}
