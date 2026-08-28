#pragma once

#include "GameFramework/Actor.h"
#include "ChopItCombatDummy.generated.h"

class UChopItHealthComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class CHOPITCOMBAT_API AChopItCombatDummy final : public AActor
{
	GENERATED_BODY()

public:
	AChopItCombatDummy();
	UChopItHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	virtual void BeginPlay() override;

private:
	void HandleHealthChanged(float CurrentHealth, float MaxHealth, AActor* DamageSource);
	void HandleDeath(AActor* DeadActor, AActor* DamageSource);
	void UpdateHealthLabel(float CurrentHealth, float MaxHealth);

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Dummy")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Dummy")
	TObjectPtr<UTextRenderComponent> HealthLabel;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Dummy")
	TObjectPtr<UChopItHealthComponent> HealthComponent;
};
