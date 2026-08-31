#pragma once

#include "Engine/DataAsset.h"
#include "ChopItChainDefinition.generated.h"

class UStaticMesh;

/**
 * Complete tuning preset for the retractable player chain.
 * One definition can be shared by every quota machine or swapped per level.
 */
UCLASS(BlueprintType)
class CHOPITWORLD_API UChopItChainDefinition final : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|01 Master")
	bool bChainPlayerToMachine = true;

	/** Maximum number of visible links stored by the machine. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|02 Links", meta = (ClampMin = "3", ClampMax = "64"))
	int32 ChainLinkCount = 48;

	/** Total physical length stored in the machine. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|02 Links", meta = (ClampMin = "100.0", Units = "cm"))
	float MaxChainLength = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|02 Links", meta = (ClampMin = "3", ClampMax = "16"))
	int32 MinimumDeployedLinks = 11;

	/** Scales cable gravity relative to the 1.25 kg reference weight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|02 Links", meta = (ClampMin = "0.01", Units = "kg"))
	float ChainLinkWeight = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|03 Reel", meta = (ClampMin = "20.0", ClampMax = "2000.0", Units = "cm/s"))
	float ChainFeedSpeed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|03 Reel", meta = (ClampMin = "20.0", ClampMax = "8000.0", Units = "cm/s^2"))
	float ChainFeedAcceleration = 3200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|03 Reel", meta = (ClampMin = "0.0", Units = "cm"))
	float ChainSlack = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|03 Reel", meta = (ClampMin = "0.0", Units = "cm"))
	float ChainReelHysteresis = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|03 Reel", meta = (ClampMin = "1.0", ClampMax = "50.0", Units = "cm"))
	float ChainStretchTolerance = 10.0f;

	/** Collision particles per visible link. More particles improve wrapping around trunks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|04 Simulation", meta = (ClampMin = "2", ClampMax = "6"))
	int32 CableSegmentsPerLink = 4;

	/** Minimum rigidity passes per fixed simulation step. Long ropes automatically add enough passes to avoid rubber-band stretch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|04 Simulation", meta = (ClampMin = "1", ClampMax = "64"))
	int32 CableSolverIterations = 32;

	/** Removes velocity created only by length corrections. One is inelastic; zero preserves all spring-like rebound. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|04 Simulation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CableConstraintVelocityDamping = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|04 Simulation", meta = (ClampMin = "0.005", ClampMax = "0.033", Units = "s"))
	float CableSubstepTime = 0.008333f;

	/** Caps catch-up work after a slow frame instead of making the rope explode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|04 Simulation", meta = (ClampMin = "1", ClampMax = "16"))
	int32 CableMaximumSubsteps = 8;

	/** Removes a small amount of velocity each step without making deployment feel heavy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|04 Simulation", meta = (ClampMin = "0.0", ClampMax = "0.25"))
	float CableVelocityDamping = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|04 Simulation", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float CableGravityScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision")
	bool bCableWorldCollision = true;

	/** Requested swept-particle diameter. The solver raises it when necessary so collision never has gaps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision", meta = (ClampMin = "2.0", Units = "cm"))
	float CableParticleDiameter = 12.5f;

	/** Extra separation after a collision sweep, useful against numerical clipping. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision", meta = (ClampMin = "0.1", ClampMax = "5.0", Units = "cm"))
	float CableCollisionSkin = 1.0f;

	/** Collision passes interleaved with constraint solving. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision", meta = (ClampMin = "1", ClampMax = "8"))
	int32 CableCollisionIterations = 5;

	/** Keep at zero so deployed rope can slide freely over the floor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CableGroundFriction = 0.0f;

	/** Side friction against trunks, rocks and walls so the rope can remain wrapped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CableCollisionFriction = 0.08f;

	/** Radius used by the authoritative route sweeps. This is independent from the visual particles. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision", meta = (ClampMin = "1.0", Units = "cm"))
	float WrapSweepRadius = 6.0f;

	/** Keeps a wrap point just outside the contacted surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision", meta = (ClampMin = "0.1", Units = "cm"))
	float WrapAnchorSurfaceOffset = 0.75f;

	/** Contacts closer than this on the same component update one anchor instead of creating a duplicate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision", meta = (ClampMin = "1.0", Units = "cm"))
	float WrapMinimumAnchorSeparation = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision", meta = (ClampMin = "1", ClampMax = "32"))
	int32 MaximumWrapAnchors = 32;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaximumWrapInsertionsPerFrame = 4;

	/** A direct path must remain clear for this many frames before a bend is removed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision", meta = (ClampMin = "1", ClampMax = "12"))
	int32 UnwrapConfirmationFrames = 2;

	/** Fraction of visual constraints solved each XPBD pass. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|04 Simulation", meta = (ClampMin = "0.5", ClampMax = "1.0"))
	float CableXPBDStiffness = 1.0f;

	/** Minimum particle count allocated to every independently simulated route span. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|04 Simulation", meta = (ClampMin = "3", ClampMax = "64"))
	int32 MinimumVisualParticlesPerSpan = 5;

	/** The final part of the chain blends into tension over this distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|09 Gameplay Tension", meta = (ClampMin = "1.0", Units = "cm"))
	float TensionSoftBand = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|09 Gameplay Tension", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float PlayerPullAcceleration = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|09 Gameplay Tension", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float PlayerPullDamping = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|09 Gameplay Tension", meta = (ClampMin = "0.0"))
	float MaximumPropTensionForce = 250000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|09 Gameplay Tension", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float PhysicsPropForceScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|10 Debug")
	bool bDebugDrawTether = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|06 Visual")
	TObjectPtr<UStaticMesh> ChainLinkMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|06 Visual", meta = (ClampMin = "10.0", Units = "cm"))
	float ChainLinkLength = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|06 Visual", meta = (ClampMin = "2.0", Units = "cm"))
	float ChainLinkThickness = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|06 Visual", meta = (ClampMin = "0.0", ClampMax = "20.0", Units = "cm"))
	float ChainLinkVisualOverlap = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|08 Anchors")
	FVector MachineChainAnchor = FVector(0.0f, 0.0f, 150.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|08 Anchors")
	FVector PlayerChainAnchor = FVector(0.0f, 0.0f, -45.0f);
};
