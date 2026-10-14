#pragma once

#include "UObject/Object.h"
#include "TimerManager.h"
#include "Phone/ChopItPhoneStateComponent.h"
#include "ChopItPhonePromptBinding.generated.h"

class AChopItMysteryPhone;
class APlayerController;
class APawn;
class UPrimitiveComponent;
class USphereComponent;
class UWidgetComponent;
class UChopItPhonePromptConfig;
class UChopItPhonePromptViewModel;

/** Local presentation adapter. The gameplay interaction remains in ChopItWorld. */
UCLASS()
class CHOPIT_API UChopItPhonePromptBinding final : public UObject
{
	GENERATED_BODY()
public:
	void Initialize(APlayerController* InPlayer, AChopItMysteryPhone* InPhone, UChopItPhonePromptConfig* InConfig);
	void Shutdown();
	void RefreshPresentation() { RefreshPrompt(); }
	AChopItMysteryPhone* GetPhone() const { return Phone.Get(); }
	UChopItPhonePromptViewModel* GetViewModel() const { return ViewModel; }
	UWidgetComponent* GetWidgetComponent() const { return PromptWidget; }
	static bool CanShowFor(const AChopItMysteryPhone* InPhone, const APawn* InPawn);

private:
	UFUNCTION() void HandleOverlapBegin(UPrimitiveComponent* Overlapped, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION() void HandleOverlapEnd(UPrimitiveComponent* Overlapped, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	UFUNCTION() void HandleStateChanged(EChopItPhoneState Previous, EChopItPhoneState Current);
	void CheckNearbyPlayer();
	void RefreshPrompt();
	void HidePrompt();
	void StopCheck();

	TWeakObjectPtr<APlayerController> Player;
	TWeakObjectPtr<AChopItMysteryPhone> Phone;
	UPROPERTY(Transient) TObjectPtr<UChopItPhonePromptConfig> Config;
	UPROPERTY(Transient) TObjectPtr<UChopItPhonePromptViewModel> ViewModel;
	UPROPERTY(Transient) TObjectPtr<USphereComponent> DetectionSphere;
	UPROPERTY(Transient) TObjectPtr<UWidgetComponent> PromptWidget;
	FTimerHandle CheckTimer;
};
