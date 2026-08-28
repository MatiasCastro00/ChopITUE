#pragma once

#include "GameFramework/Character.h"
#include "ChopItEnemyCharacter.generated.h"

class UChopItEnemyDefinition;
class UChopItHealthComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UChopItHitFeedbackComponent;

/** Lightweight, direct-chase enemy used by the first horde slice. */
UCLASS(Blueprintable)
class CHOPITAI_API AChopItEnemyCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	AChopItEnemyCharacter();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeFromDefinition(UChopItEnemyDefinition* NewDefinition, AActor* NewTarget);
	const UChopItEnemyDefinition* GetDefinition() const { return Definition; }
	UChopItHealthComponent* GetHealthComponent() const { return HealthComponent; }
protected:
	virtual void BeginPlay() override;
private:
	void HandleDeath(AActor* DeadActor, AActor* DamageSource);
	void AwardExperience();
	void UpdateLabel() const;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Enemy") TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Enemy") TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Enemy") TObjectPtr<UChopItHealthComponent> HealthComponent;
	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Enemy") TObjectPtr<UChopItHitFeedbackComponent> HitFeedbackComponent;
	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Enemy") TObjectPtr<UChopItEnemyDefinition> Definition;
	TWeakObjectPtr<AActor> TargetActor;
	double NextAttackAt = 0.0;
	bool bRewardGranted = false;
};
