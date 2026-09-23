#include "Economy/ChopItChainDefinition.h"

void UChopItChainDefinition::MigrateToV2()
{
	if (PhysicsVersion >= 2) return;
	// Preserve authored length, anchors, reel speeds, collision settings and mesh.
	PhysicalSegmentLength = FMath::Clamp(PhysicalSegmentLength, 4.0f, 30.0f);
	MinimumDeployedLength = FMath::Clamp(MinimumDeployedLength, 20.0f, FMath::Max(20.0f, MaxChainLength));
	PhysicsVersion = 2;
}

void UChopItChainDefinition::PostLoad()
{
	Super::PostLoad();
	MigrateToV2();
}
