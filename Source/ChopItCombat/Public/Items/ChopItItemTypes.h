#pragma once

#include "CoreMinimal.h"
#include "ChopItItemTypes.generated.h"

class UChopItItemDataAsset;

UENUM(BlueprintType)
enum class EChopItItemRarity : uint8 { Common, Uncommon, Rare, Legendary };

UENUM(BlueprintType)
enum class EChopItItemEvent : uint8
{
	DamageDealt, DamageReceived, EnemyKilled, TreeHit, TreeDestroyed,
	WoodCollected, WoodDelivered, DayStarted, NightStarted, ItemPurchased,
	InfestationChanged, InfestationExecuted
};

/** Amount is actual damage/wood, or accumulated infestation. Recipient null means world-wide. */
USTRUCT(BlueprintType)
struct CHOPITCOMBAT_API FChopItItemEventContext
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EChopItItemEvent Event = EChopItItemEvent::DamageDealt;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> Recipient = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> Source = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> Target = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Amount = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Location = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bExecution = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCritical = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Generation = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UChopItItemDataAsset> Item = nullptr;
	/** Also identifies purchases of existing weapons, without a dependency on their definition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ContentId;
};
