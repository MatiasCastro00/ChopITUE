#pragma once
#include "IDetailCustomization.h"

class FChopItTerrainAssetCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShared<FChopItTerrainAssetCustomization>(); }
	virtual void CustomizeDetails(IDetailLayoutBuilder& Builder) override;
};
