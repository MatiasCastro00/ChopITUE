#pragma once

#include "GameFramework/Actor.h"
#include "ChopItBossRewardChest.generated.h"

class UChopItItemComponent;
class UChopItItemDataAsset;
class UChopItCameraFacingTextComponent;
class UNiagaraComponent;
class UPointLightComponent;
class USphereComponent;
class UStaticMeshComponent;

/** World reward for a defeated elite. The loot subsystem chooses the item. */
UCLASS()
class CHOPIT_API AChopItBossRewardChest final : public AActor
{
	GENERATED_BODY()
public:
	AChopItBossRewardChest();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UFUNCTION()
	void HandleApproach(UPrimitiveComponent* Overlapped, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);
	void TryOpen(AActor* Actor);
	void GrantReward();
	void FindNearbyPlayer();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> VisualRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Approach;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Lid;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Lock;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UChopItCameraFacingTextComponent> Label;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Light;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UNiagaraComponent> ArrivalParticles;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UNiagaraComponent> OpeningParticles;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UNiagaraComponent> IdleParticles;
	UPROPERTY(Transient) TObjectPtr<UChopItItemDataAsset> ChosenItem;
	TWeakObjectPtr<UChopItItemComponent> Recipient;
	float EntranceTime = 0.f;
	float LidTime = 0.f;
	float IdleTime = 0.f;
	float IdleBurstTime = 0.f;
	bool bReady = false;
	bool bOpening = false;
	bool bGranted = false;
};
