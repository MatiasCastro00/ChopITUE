#pragma once

#include "GameFramework/Actor.h"
#include "ChopItTree.generated.h"

class AChopItLogPickup;
class AChopItTree;
class UBoxComponent;
class UChopItHealthComponent;
class UMaterialInterface;
class UStaticMeshComponent;
class UTextRenderComponent;
class UChopItHitFeedbackComponent;

UENUM(BlueprintType)
enum class EChopItTreeHarvestState : uint8
{
	Standing,
	Falling,
	Settled,
	Harvested
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FChopItTreeHarvestedNative, AChopItTree*, int32, int32);

/** Owns the authoritative tree harvest state and emits its reward exactly once. */
UCLASS(Blueprintable)
class CHOPITWORLD_API AChopItTree : public AActor
{
	GENERATED_BODY()

public:
	AChopItTree();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void SetBlockoutMaterials(UMaterialInterface* TrunkMaterial, UMaterialInterface* CrownMaterial);
	UChopItHealthComponent* GetHealthComponent() const { return HealthComponent; }
	UBoxComponent* GetPhysicsRoot() const { return PhysicsRoot; }
	EChopItTreeHarvestState GetHarvestState() const { return HarvestState; }
	bool HasSpawnedReward() const { return bRewardSpawned; }

	FChopItTreeHarvestedNative OnHarvested;

private:
	void HandleHealthChanged(float CurrentHealth, float MaxHealth, AActor* DamageSource);
	void HandleDepleted(AActor* DeadActor, AActor* DamageSource);
	void CheckFallSettled();
	void SettleAndSpawnReward();
	void SpawnRewardOnce();
	void UpdateHealthLabel(float CurrentHealth, float MaxHealth);

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Tree")
	TObjectPtr<UBoxComponent> PhysicsRoot;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Tree")
	TObjectPtr<UStaticMeshComponent> TrunkMesh;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Tree")
	TObjectPtr<UStaticMeshComponent> CrownMesh;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Tree")
	TObjectPtr<UTextRenderComponent> HealthLabel;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Tree")
	TObjectPtr<UChopItHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Tree")
	TObjectPtr<UChopItHitFeedbackComponent> HitFeedbackComponent;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Harvest")
	TSubclassOf<AChopItLogPickup> LogPickupClass;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Harvest", meta = (ClampMin = "1"))
	int32 WoodRewardUnits = 3;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Harvest", meta = (ClampMin = "0"))
	int32 ExperienceReward = 5;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Harvest", meta = (ClampMin = "0.1"))
	float MinimumFallDuration = 0.8f;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Harvest", meta = (ClampMin = "0.2"))
	float MaximumFallDuration = 3.5f;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Harvest")
	EChopItTreeHarvestState HarvestState = EChopItTreeHarvestState::Standing;

	bool bRewardSpawned = false;
	TWeakObjectPtr<AActor> RewardRecipient;
	double FallStartedAt = 0.0;
	FTimerHandle FallSettleTimerHandle;
};
