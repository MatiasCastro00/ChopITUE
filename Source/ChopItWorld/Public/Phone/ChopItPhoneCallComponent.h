#pragma once

#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ChopItPhoneCallComponent.generated.h"

class UAudioComponent;
class UChopItPhoneCallConfig;
class UChopItPhoneStateComponent;
enum class EChopItPhoneState : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FChopItPhoneRingEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChopItPhoneRingBeatEvent, int32, BeatIndex);

/** Owns a single reusable spatial audio component and timer-based call cycle. */
UCLASS(ClassGroup=(ChopIt), BlueprintType, meta=(BlueprintSpawnableComponent))
class CHOPITWORLD_API UChopItPhoneCallComponent final : public UActorComponent
{
	GENERATED_BODY()
public:
	UChopItPhoneCallComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phone Call")
	TSoftObjectPtr<UChopItPhoneCallConfig> CallConfig;
	UPROPERTY(BlueprintAssignable, Category="Phone Call")
	FChopItPhoneRingEvent OnRingStarted;
	UPROPERTY(BlueprintAssignable, Category="Phone Call")
	FChopItPhoneRingEvent OnRingStopped;
	UPROPERTY(BlueprintAssignable, Category="Phone Call")
	FChopItPhoneRingBeatEvent OnRingBeat;

	/** Can also be called by an authored event; the same guards apply as for scheduled calls. */
	UFUNCTION(BlueprintCallable, Category="ChopIt|Phone|Call")
	bool StartCall();
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone|Call")
	bool IsCallScheduled() const;
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone|Call")
	UAudioComponent* GetRingAudio() const { return RingAudio; }
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone|Call")
	UAudioComponent* GetSecondaryRingAudio() const { return SecondaryRingAudio; }
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone|Call")
	int32 GetEmittedBeatCount() const { return NextBeatIndex; }
	UFUNCTION(BlueprintPure, Category="ChopIt|Phone|Call")
	float GetAudibleDistance() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UChopItPhoneStateComponent> PhoneState;
	UPROPERTY(Transient)
	TObjectPtr<UChopItPhoneCallConfig> ActiveConfig;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> RingAudio;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> SecondaryRingAudio;
	FTimerHandle IntervalTimer;
	FTimerHandle DurationTimer;
	FTimerHandle PairTimer;
	FTimerHandle SecondStrikeTimer;
	int32 NextBeatIndex = 0;
	int32 NextPairIndex = 0;
	FRandomStream Random;

	void ScheduleNextCall();
	void StartScheduledCall();
	void EndCall();
	void EmitPair();
	void EmitNextBeat();
	void StopAudio();
	bool bHasRung = false;
	UFUNCTION()
	void HandleStateChanged(EChopItPhoneState Previous, EChopItPhoneState Current);
};
