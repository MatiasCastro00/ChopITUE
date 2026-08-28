#include "ChopItLogChannels.h"
#include "Modules/ModuleManager.h"

class FChopItCoreModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogChopIt, Log, TEXT("ChopItCore module started."));
	}
};

IMPLEMENT_MODULE(FChopItCoreModule, ChopItCore);
