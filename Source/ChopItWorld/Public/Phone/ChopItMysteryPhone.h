#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/ChopItInteractable.h"
#include "ChopItMysteryPhone.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UChopItPhoneStateComponent;
class UChopItPhoneCallComponent;
class UChopItPhoneVariantComponent;
class UChopItPhoneRevealComponent;
class UChopItPhoneRingMotionComponent;

/** World phone shell. Its state and interaction are independent of spawn and UI. */
UCLASS(Blueprintable)
class CHOPITWORLD_API AChopItMysteryPhone : public AActor, public IChopItInteractable
{
	GENERATED_BODY()
public:
	AChopItMysteryPhone();
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintPure, Category="ChopIt|Phone")
	UChopItPhoneStateComponent* GetPhoneState() const { return PhoneState; }
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone")
	UChopItPhoneCallComponent* GetPhoneCall() const { return PhoneCall; }
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone")
	UChopItPhoneVariantComponent* GetPhoneVariant() const { return PhoneVariant; }
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone")
	UChopItPhoneRevealComponent* GetPhoneReveal() const { return PhoneReveal; }
	UStaticMeshComponent* GetBaseVisual() const { return BaseVisual; }
	UStaticMeshComponent* GetReceiverVisual() const { return ReceiverVisual; }
	USphereComponent* GetPromptDetection() const { return PromptDetection; }
	UWidgetComponent* GetPromptWidget() const { return PromptWidget; }
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone|Range")
	float GetHearingRange() const { return HearingRange; }
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone|Range")
	USphereComponent* GetHearingRangeGizmo() const { return HearingRangeGizmo; }
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Emitted after the one valid answer; later phases attach transformation and dialogue. */
	UFUNCTION(BlueprintImplementableEvent, Category="ChopIt|Phone")
	void OnAnswered(AActor* Interactor);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Phone Interaction", meta=(ClampMin="1", Units="cm"))
	float InteractionRange = 250.0f;
	/** Shared radius for audible rings, visible comic and the editor gizmo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phone Hearing Range", meta=(ClampMin="120.0", Units="cm"))
	float HearingRange = 850.0f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Phone Components")
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Phone Components")
	TObjectPtr<USphereComponent> InteractionBody;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Phone Components")
	TObjectPtr<USphereComponent> HearingRangeGizmo;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Phone Components")
	TObjectPtr<USphereComponent> PromptDetection;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Phone Components")
	TObjectPtr<UWidgetComponent> PromptWidget;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Phone Components")
	TObjectPtr<UStaticMeshComponent> BaseVisual;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Phone Components")
	TObjectPtr<UStaticMeshComponent> ReceiverVisual;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Phone Components")
	TObjectPtr<UChopItPhoneStateComponent> PhoneState;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Phone Components")
	TObjectPtr<UChopItPhoneCallComponent> PhoneCall;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Phone Components")
	TObjectPtr<UChopItPhoneVariantComponent> PhoneVariant;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Phone Components")
	TObjectPtr<UChopItPhoneRevealComponent> PhoneReveal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Phone Components")
	TObjectPtr<UChopItPhoneRingMotionComponent> PhoneRingMotion;
};
