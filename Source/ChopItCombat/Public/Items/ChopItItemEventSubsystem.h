#pragma once
#include "Items/ChopItItemTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "ChopItItemEventSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FChopItItemEventNative, const FChopItItemEventContext&);

/** Small event boundary shared by combat/world systems; knows no concrete items. */
UCLASS()
class CHOPITCOMBAT_API UChopItItemEventSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	FChopItItemEventNative OnEvent;
	UFUNCTION(BlueprintCallable, Category="ChopIt|Items")
	void Publish(const FChopItItemEventContext& Context) { OnEvent.Broadcast(Context); }
	static void Emit(const UObject* WorldContext, const FChopItItemEventContext& Context);
};
