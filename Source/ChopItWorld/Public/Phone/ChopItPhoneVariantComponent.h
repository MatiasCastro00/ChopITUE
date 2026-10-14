#pragma once

#include "Components/ActorComponent.h"
#include "Phone/ChopItPhoneStateComponent.h"
#include "ChopItPhoneVariantComponent.generated.h"

class UChopItPhoneVariantData;
class UChopItPhoneVariantPool;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChopItPhoneVariantSelected, UChopItPhoneVariantData*, Variant);

/** Selects an immutable variant once when the answered state is entered. */
UCLASS(ClassGroup=(ChopIt), BlueprintType, meta=(BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItPhoneVariantComponent final : public UActorComponent
{
	GENERATED_BODY()
public:
	UChopItPhoneVariantComponent();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phone Variant")
	TSoftObjectPtr<UChopItPhoneVariantPool> VariantPool;
	UPROPERTY(BlueprintAssignable, Category="Phone Variant")
	FChopItPhoneVariantSelected OnVariantSelected;
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone|Variant")
	UChopItPhoneVariantData* GetSelectedVariant() const { return SelectedVariant; }
	void InitializeSelectionSeed(int32 Seed);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION() void HandleStateChanged(EChopItPhoneState Previous, EChopItPhoneState Current);
	void SelectAfterAnswer();
	UPROPERTY(Transient) TObjectPtr<UChopItPhoneStateComponent> PhoneState;
	UPROPERTY(Transient) TObjectPtr<UChopItPhoneVariantPool> ActivePool;
	UPROPERTY(VisibleInstanceOnly, Category="Phone Variant")
	TObjectPtr<UChopItPhoneVariantData> SelectedVariant;
	int32 SelectionSeed = 0;
	bool bHasSelectionSeed = false;
};
