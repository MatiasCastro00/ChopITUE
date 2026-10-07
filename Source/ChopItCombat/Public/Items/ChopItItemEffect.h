#pragma once
#include "Items/ChopItItemTypes.h"
#include "ChopItItemEffect.generated.h"

class UChopItItemComponent;

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class CHOPITCOMBAT_API UChopItItemEffect : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") TArray<EChopItItemEvent> Events;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") float BaseValue = 0.1f;
	UPROPERTY(Transient, BlueprintReadOnly, Category="Runtime") TObjectPtr<UChopItItemComponent> Inventory;
	UPROPERTY(Transient, BlueprintReadOnly, Category="Runtime") TObjectPtr<UChopItItemDataAsset> Definition;
	UPROPERTY(Transient, BlueprintReadOnly, Category="Runtime") int32 Stacks = 0;
	virtual UWorld* GetWorld() const override;
	UFUNCTION(BlueprintPure, Category="ChopIt|Items") float GetStackedValue() const;
	UFUNCTION(BlueprintNativeEvent, Category="ChopIt|Items") void OnActivated();
	virtual void OnActivated_Implementation() {}
	UFUNCTION(BlueprintNativeEvent, Category="ChopIt|Items") void OnStacksChanged();
	virtual void OnStacksChanged_Implementation() {}
	UFUNCTION(BlueprintNativeEvent, Category="ChopIt|Items") void OnDeactivated();
	virtual void OnDeactivated_Implementation() {}
	UFUNCTION(BlueprintNativeEvent, Category="ChopIt|Items") void HandleEvent(const FChopItItemEventContext& Context);
	virtual void HandleEvent_Implementation(const FChopItItemEventContext& Context) {}
};
