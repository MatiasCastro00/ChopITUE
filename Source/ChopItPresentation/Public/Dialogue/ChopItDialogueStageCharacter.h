#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChopItDialogueStageCharacter.generated.h"

class UBillboardComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UTexture2D;

/**
 * Lightweight portrait actor used by dialogue cinematics until a full 3D NPC is available.
 * It idles and reacts in real time, including while a modal dialogue has paused the world.
 */
UCLASS()
class CHOPITPRESENTATION_API AChopItDialogueStageCharacter final : public AActor
{
	GENERATED_BODY()

public:
	AChopItDialogueStageCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void Configure(UTexture2D* PortraitTexture, float Height = 260.0f);
	void ConfigurePillMarker(const FText& DisplayName);
	void PlayReaction(FName ReactionId);
	void BeginExit();

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBillboardComponent> Portrait;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PillBody;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PillTop;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PillBottom;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> NameLabel;

	FVector RestLocation = FVector::ZeroVector;
	float IdleTime = 0.0f;
	float ReactionTime = 0.0f;
	float ExitTime = 0.0f;
	float BaseScale = 1.0f;
	FName ActiveReaction;
	bool bExiting = false;
	bool bUsePillMarker = false;
};
