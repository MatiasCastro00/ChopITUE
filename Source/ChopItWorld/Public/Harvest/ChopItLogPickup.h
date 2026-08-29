#pragma once

#include "GameFramework/Actor.h"
#include "ChopItLogPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UChopItWoodCargoComponent;

UCLASS(Blueprintable)
class CHOPITWORLD_API AChopItLogPickup : public AActor
{
	GENERATED_BODY()

public:
	AChopItLogPickup();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void InitializeWoodUnits(int32 NewWoodUnits);
	void InitializeReward(int32 NewWoodUnits, int32 NewExperienceReward);
	void LaunchFromImpact(const FVector& LinearVelocity, const FVector& AngularVelocityDegrees);
	int32 GetWoodUnits() const { return WoodUnits; }
	USphereComponent* GetMagnetSphere() const { return MagnetSphere; }
	USphereComponent* GetPhysicsBody() const { return PhysicsBody; }

private:
	UFUNCTION()
	void HandleMagnetBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleMagnetEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	void UpdateMagnetism();
	void UpdateGroundSafety();
	void UpdateLabel();
	void UpdateLabelFacingCamera();
	void ConfigureDroppedPhysics();
	void ResumeDroppedPhysics();

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Pickup")
	TObjectPtr<USphereComponent> PhysicsBody;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Pickup")
	TObjectPtr<USphereComponent> MagnetSphere;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Pickup")
	TObjectPtr<UStaticMeshComponent> LogMesh;

	UPROPERTY(VisibleAnywhere, Category = "ChopIt|Pickup")
	TObjectPtr<UTextRenderComponent> UnitLabel;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Pickup", meta = (ClampMin = "1"))
	int32 WoodUnits = 3;

	UPROPERTY(VisibleInstanceOnly, Category = "ChopIt|Pickup")
	int32 ExperienceReward = 0;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Pickup", meta = (ClampMin = "1.0"))
	float MagnetSpeed = 900.0f;

	UPROPERTY(EditAnywhere, Category = "ChopIt|Pickup", meta = (ClampMin = "1.0"))
	float CollectionDistance = 100.0f;

	TWeakObjectPtr<UChopItWoodCargoComponent> CandidateCargo;
	FTimerHandle MagnetTimerHandle;
	FTimerHandle GroundSafetyTimerHandle;
	FVector PreviousPhysicsLocation = FVector::ZeroVector;
};
