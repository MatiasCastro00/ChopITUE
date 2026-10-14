#pragma once

#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"

class UChopItTerrainEditorSettings;

/** Per-tab decorations for every terrain control. */
class FChopItTerrainDetailsCustomization final : public IDetailCustomization
{
public:
	explicit FChopItTerrainDetailsCustomization(UChopItTerrainEditorSettings* InSettings) : Settings(InSettings) {}
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	TWeakObjectPtr<UChopItTerrainEditorSettings> Settings;
};

class FChopItTerrainPropertyTypeCustomization final : public IPropertyTypeCustomization
{
public:
	FChopItTerrainPropertyTypeCustomization(UChopItTerrainEditorSettings* InSettings, FString InPrefix)
		: Settings(InSettings), Prefix(MoveTemp(InPrefix)) {}
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	TWeakObjectPtr<UChopItTerrainEditorSettings> Settings;
	FString Prefix;
};
