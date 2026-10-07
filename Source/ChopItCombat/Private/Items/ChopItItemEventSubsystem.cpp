#include "Items/ChopItItemEventSubsystem.h"
#include "Engine/World.h"

void UChopItItemEventSubsystem::Emit(const UObject* WorldContext, const FChopItItemEventContext& Context)
{
	if (UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
	{
		if (auto* Events = World->GetSubsystem<UChopItItemEventSubsystem>()) Events->Publish(Context);
	}
}
