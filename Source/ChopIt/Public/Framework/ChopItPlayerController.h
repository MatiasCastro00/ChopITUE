#pragma once

#include "GameFramework/PlayerController.h"
#include "ChopItPlayerController.generated.h"

/** Owns local input context and interaction intent, not gameplay rules. */
UCLASS()
class CHOPIT_API AChopItPlayerController final : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void SelectUpgradeOne();
	void SelectUpgradeTwo();
	void SelectUpgradeThree();
	void SelectUpgrade(int32 Index);
	void CloseShop();
};
