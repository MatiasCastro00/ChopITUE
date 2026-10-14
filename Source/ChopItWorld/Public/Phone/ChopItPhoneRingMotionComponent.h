#pragma once

#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ChopItPhoneRingMotionComponent.generated.h"

class AChopItMysteryPhone;
class UChopItPhoneCallComponent;

/** Rings the visible shell and handset while the phone call is active. */
UCLASS(ClassGroup=(ChopIt), BlueprintType, meta=(BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItPhoneRingMotionComponent final : public UActorComponent
{
	GENERATED_BODY()
public:
	UChopItPhoneRingMotionComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phone Ring Motion", meta=(ClampMin="0.01", Units="s"))
	float UpdatePeriod = 0.016f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phone Ring Motion", meta=(ClampMin="0.05", Units="s"))
	float BeatDurationSeconds = 0.28f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phone Ring Motion", meta=(ClampMin="0", Units="deg"))
	float ShellRockDegrees = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phone Ring Motion", meta=(ClampMin="0", Units="deg"))
	float ReceiverRockDegrees = 9.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phone Ring Motion", meta=(ClampMin="0", Units="cm"))
	float ShellBobCm = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phone Ring Motion", meta=(ClampMin="0", Units="cm"))
	float ReceiverHopCm = 4.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION() void HandleRingStarted();
	UFUNCTION() void HandleRingStopped();
	UFUNCTION() void HandleRingBeat(int32 BeatIndex);
	void UpdateMotion();
	void RestoreVisuals();

	TWeakObjectPtr<AChopItMysteryPhone> Phone;
	TWeakObjectPtr<UChopItPhoneCallComponent> Call;
	FTransform OriginalShell;
	FTransform OriginalReceiver;
	FTimerHandle MotionTimer;
	float Elapsed = 0.0f;
};
