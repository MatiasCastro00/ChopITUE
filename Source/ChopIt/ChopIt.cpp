// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChopIt.h"
#include "ChopItLogChannels.h"
#include "Modules/ModuleManager.h"

class FChopItModule final : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		UE_LOG(LogChopIt, Log, TEXT("ChopIt runtime composition module started."));
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FChopItModule, ChopIt, "ChopIt");
