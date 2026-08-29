#pragma once

#include "GameFramework/Actor.h"
#include "ChopItTree.generated.h"

class AChopItLogPickup;
class AChopItTree;
class UBoxComponent;
class UChopItHealthComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UChopItHitFeedbackComponent;

UENUM(BlueprintType)
enum class EChopItTreeFoliageVariant : uint8
{
	Auto UMETA(DisplayName = "Auto"),
	Pine UMETA(DisplayName = "Pine"),
	Spring UMETA(DisplayName = "Spring"),
	Summer UMETA(DisplayName = "Summer"),
	Autumn UMETA(DisplayName = "Autumn")
};

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
	void SetFoliageVariant(EChopItTreeFoliageVariant NewVariant);
	UChopItHealthComponent* GetHealthComponent() const { return HealthComponent; }
	UBoxComponent* GetPhysicsRoot() const { return PhysicsRoot; }
	USphereComponent* GetCrownCollision() const { return CrownCollision; }
	EChopItTreeHarvestState GetHarvestState() const { return HarvestState; }
	bool HasSpawnedReward() const { return bRewardSpawned; }

	FChopItTreeHarvestedNative OnHarvested;

private:
	void HandleHealthChanged(float CurrentHealth, float MaxHealth, AActor* DamageSource);
	void HandleDepleted(AActor* DeadActor, AActor* DamageSource);

	UFUNCTION()
	void HandleFallImpact(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	void CheckFallSettled();
	bool TryResolveCrownContact();
	bool IsImpactOnCrown(const FHitResult& Hit) const;
	FLinearColor GetFoliageColor() const;
	void ApplyFoliageColor();
	void SpawnRewardOnce(const FVector& SpawnOrigin, const FVector& ImpactNormal);
	void DestroyAtImpact(const FVector& ImpactLocation, const FVector& ImpactNormal);
	void UpdateHealthLabel(float CurrentHealth, float MaxHealth);

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Tree")
	TObjectPtr<UBoxComponent> PhysicsRoot;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Tree")
	TObjectPtr<UStaticMeshComponent> TrunkMesh;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Tree")
	TObjectPtr<UStaticMeshComponent> CrownMesh;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Tree")
	TObjectPtr<USphereComponent> CrownCollision;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Tree")
	TObjectPtr<UTextRenderComponent> HealthLabel;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Tree")
	TObjectPtr<UChopItHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Tree")
	TObjectPtr<UChopItHitFeedbackComponent> HitFeedbackComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Visual", meta = (AllowPrivateAccess = "true"))
	EChopItTreeFoliageVariant FoliageVariant = EChopItTreeFoliageVariant::Auto;

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
	TObjectPtr<UMaterialInterface> CrownMaterialSource;
	TObjectPtr<UMaterialInstanceDynamic> CrownMaterialInstance;
	double FallStartedAt = 0.0;
	FVector PreviousCrownLocation = FVector::ZeroVector;
	FTimerHandle FallSettleTimerHandle;
};
