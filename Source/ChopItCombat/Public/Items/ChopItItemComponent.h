#pragma once
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Items/ChopItItemTypes.h"
#include "ChopItItemComponent.generated.h"

class UChopItItemEffect;

USTRUCT(BlueprintType)
struct CHOPITCOMBAT_API FChopItOwnedItem
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UChopItItemDataAsset> Item = nullptr;
	UPROPERTY(BlueprintReadOnly) int32 Stacks = 0;
	UPROPERTY(Transient, BlueprintReadOnly) TArray<TObjectPtr<UChopItItemEffect>> Effects;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChopItInventoryChanged, UChopItItemDataAsset*, Item, int32, Stacks);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChopItItemEventObserved, const FChopItItemEventContext&, Context);

UCLASS(BlueprintType, ClassGroup=(ChopIt), meta=(BlueprintSpawnableComponent))
class CHOPITCOMBAT_API UChopItItemComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UChopItItemComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UFUNCTION(BlueprintCallable, Category="ChopIt|Items") bool AddItem(UChopItItemDataAsset* Item, int32 Count = 1);
	UFUNCTION(BlueprintCallable, Category="ChopIt|Items") bool RemoveItem(FName ItemId, int32 Count = 1);
	UFUNCTION(BlueprintCallable, Category="ChopIt|Items") void ClearItems();
	UFUNCTION(BlueprintPure, Category="ChopIt|Items") int32 GetStackCount(FName ItemId) const;
	UFUNCTION(BlueprintPure, Category="ChopIt|Items") TArray<FChopItOwnedItem> GetItems() const;
	UFUNCTION(BlueprintPure, Category="ChopIt|Items") FGameplayTagContainer GetOwnedTags() const;
	UFUNCTION(BlueprintPure, Category="ChopIt|Items") static float CalculateStackedValue(float BaseValue, int32 StackCount, float StackDecay = 0.5f);
	UPROPERTY(BlueprintAssignable) FChopItInventoryChanged OnInventoryChanged;
	UPROPERTY(BlueprintAssignable) FChopItItemEventObserved OnItemEvent;
private:
	void ReceiveEvent(const FChopItItemEventContext& Context);
	void ActivateEffects(FName Id);
	UPROPERTY(Transient) TMap<FName, FChopItOwnedItem> OwnedItems;
	bool bMutating = false;
	bool bClearing = false;
	bool bEndingPlay = false;
};
