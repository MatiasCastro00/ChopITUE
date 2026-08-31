#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

/** Keep C++ collision use aligned with Config/DefaultEngine.ini. */
namespace ChopItCollisionChannels
{
	inline constexpr ECollisionChannel Enemy = ECC_GameTraceChannel1;
	inline constexpr ECollisionChannel Harvestable = ECC_GameTraceChannel2;
	inline constexpr ECollisionChannel Projectile = ECC_GameTraceChannel3;
	inline constexpr ECollisionChannel Pickup = ECC_GameTraceChannel4;
	inline constexpr ECollisionChannel DeliveryZone = ECC_GameTraceChannel5;
	inline constexpr ECollisionChannel Interaction = ECC_GameTraceChannel6;
	inline constexpr ECollisionChannel Chain = ECC_GameTraceChannel7;
	inline constexpr ECollisionChannel CameraSolid = ECC_GameTraceChannel8;
	inline constexpr ECollisionChannel CameraOcclusion = ECC_GameTraceChannel9;
}

namespace ChopItCollisionProfiles
{
	inline const FName Player(TEXT("ChopItPlayer"));
	inline const FName Enemy(TEXT("ChopItEnemy"));
	inline const FName Harvestable(TEXT("ChopItHarvestable"));
	inline const FName Projectile(TEXT("ChopItProjectile"));
	inline const FName Pickup(TEXT("ChopItPickup"));
	inline const FName DeliveryZone(TEXT("ChopItDeliveryZone"));
	inline const FName Chain(TEXT("ChopItChain"));
}
