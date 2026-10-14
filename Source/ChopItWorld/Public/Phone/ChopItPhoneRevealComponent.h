#pragma once

#include "Components/ActorComponent.h"
#include "Phone/ChopItPhoneStateComponent.h"
#include "TimerManager.h"
#include "ChopItPhoneRevealComponent.generated.h"

class UChopItPhoneVariantData;
class UChopItPhoneVariantComponent;
class UPointLightComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChopItPhoneRevealFinished, UChopItPhoneVariantData*, Variant);

/** Plays the data-driven reveal and advances to Dialogue when it finishes. */
UCLASS(ClassGroup=(ChopIt), BlueprintType, meta=(BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItPhoneRevealComponent final : public UActorComponent
{
	GENERATED_BODY()
public:
	UChopItPhoneRevealComponent();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Phone Reveal", meta=(ClampMin="0.01", Units="s"))
	float UpdatePeriod = 0.05f;
	UPROPERTY(BlueprintAssignable, Category="Phone Reveal")
	FChopItPhoneRevealFinished OnRevealFinished;
	UFUNCTION(BlueprintCallable, Category="ChopIt|Phone|Reveal")
	void CompleteRevealEarly();
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone|Reveal")
	UPointLightComponent* GetRevealLight() const { return RevealLight; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION() void HandleStateChanged(EChopItPhoneState Previous, EChopItPhoneState Current);
	void StartReveal();
	void UpdateReveal();
	void FinishReveal();
	UPROPERTY(Transient) TObjectPtr<UChopItPhoneStateComponent> PhoneState;
	UPROPERTY(Transient) TObjectPtr<UChopItPhoneVariantComponent> VariantSelection;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> RevealLight;
	FTimerHandle RevealTimer;
	float Elapsed = 0.0f;
};
