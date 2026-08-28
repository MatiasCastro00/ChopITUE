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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|04 Simulation", meta = (ClampMin = "1", ClampMax = "16"))
	int32 CableSolverIterations = 16;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|04 Simulation", meta = (ClampMin = "0.005", ClampMax = "0.033", Units = "s"))
	float CableSubstepTime = 0.008333f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|04 Simulation", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float CableGravityScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision")
	bool bCableWorldCollision = true;

	/** Diameter of each swept collision particle. Keep at least the particle spacing to avoid gaps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision", meta = (ClampMin = "2.0", Units = "cm"))
	float CableParticleDiameter = 12.5f;

	/** Zero lets the cable slide instead of snagging on ground. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChopIt|Chain|05 Collision", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CableCollisionFriction = 0.0f;

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
